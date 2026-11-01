#include <furi.h>
#include <furi_hal_usb.h>
#include <furi_hal_usb_hid.h>
#include <infrared_worker.h>
#include <furi/core/kernel.h>
#include <gui/gui.h>
#include <gui/view_port.h>
#include <gui/canvas.h>
#include <input/input.h>

#define TAG "IR_Mouse"

// リモコンのアドレス
#define REMOTE_ADDRESS 0x00007201

// NECext コマンド
#define CMD_OK          0x0000A05F
#define CMD_UP          0x0000FC03
#define CMD_DOWN        0x0000A15E
#define CMD_RIGHT       0x0000BF40
#define CMD_LEFT        0x0000FD02

#define CMD_ALT_UP      0x0000E01F
#define CMD_ALT_DOWN    0x0000E817
#define CMD_ALT_RIGHT   0x0000AB54
#define CMD_ALT_LEFT    0x0000E916

#define CMD_RIGHT_DOWN  0x0000AF50
#define CMD_RIGHT_UP    0x0000A758
#define CMD_LEFT_DOWN   0x0000ED12
#define CMD_LEFT_UP     0x0000E51A

#define CMD_RIGHT_CLICK 0x0000B34C
#define CMD_LEFT_CLICK  0x0000E41B

#define CMD_SCR_UP      0x0000B748
#define CMD_SCR_DOWN    0x0000BB44

#define CMD_BROWSER_BACK     0x0000B649
#define CMD_BROWSER_FORWARD  0x0000B54A

// 数字ボタン（テキスト入力モード用）
#define CMD_MENU       0x0000F10E
#define CMD_NUM_0      0x0000EC13
#define CMD_NUM_1      0x0000E51A
#define CMD_NUM_2      0x0000E01F
#define CMD_NUM_3      0x0000A758
#define CMD_NUM_4      0x0000E916
#define CMD_NUM_5      0x0000E41B
#define CMD_NUM_6      0x0000AB54
#define CMD_NUM_7      0x0000ED12
#define CMD_NUM_8      0x0000E817
#define CMD_NUM_9      0x0000AF50

#define CMD_BKSP       0x0000E718

#define TEXT_MULTITAP_TIMEOUT_MS 1000

// 基本マウス速度
#define MOUSE_SPEED_MIN      4
#define MOUSE_SPEED_MAX      24
#define MOUSE_SPEED_STEP     4
#define MOUSE_SPEED_DEFAULT  12

// 加速設定
#define MOUSE_ACCEL_STEP     4
#define MOUSE_ACCEL_MAX      48

// 方向識別子
#define DIR_NONE   0
#define DIR_UP     1
#define DIR_DOWN   2
#define DIR_LEFT   3
#define DIR_RIGHT  4
#define DIR_RD     5
#define DIR_RU     6
#define DIR_LD     7
#define DIR_LU     8


// ========================================
// アプリ状態
// ========================================

typedef struct {
    volatile bool running;

    // ユーザーが設定する基本速度
    volatile uint8_t mouse_speed;

    // 加速処理用
    uint8_t accelerated_speed;
    uint8_t last_direction;

    // テキスト入力モード
    volatile bool text_mode;
    volatile bool pending_valid;
    volatile uint8_t pending_digit;
    volatile uint8_t pending_index;
    volatile uint32_t pending_tick;
    volatile bool uppercase;
    volatile bool number_mode;

} IRMouseApp;


// ========================================
// USB HID設定
// ========================================

static const FuriHalUsbHidConfig hid_config = {
    .vid = 0x0483,
    .pid = 0x5740,
    .manuf = "Flipper Devices Inc.",
    .product = "Flipper IR Mouse v4.0",
};


// ========================================
// 画面描画
// ========================================

static void ir_mouse_draw_callback(
    Canvas* canvas,
    void* context) {

    IRMouseApp* app = context;

    canvas_clear(canvas);

    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str(canvas, 12, 12, "IR Mouse v4.0");

    canvas_set_font(canvas, FontSecondary);
    canvas_draw_str(canvas, 5, 24, "USB: HID Connected");

    if(app->text_mode) {
        canvas_draw_str(canvas, 5, 36, "MODE: TEXT INPUT");

        char preview[32];
        if(app->pending_valid) {
            const char* chars[] = {
                " 0", ".,?!1", "abc", "def", "ghi",
                "jkl", "mno", "pqrs", "tuv", "wxyz"
            };
            char ch = chars[app->pending_digit][app->pending_index];

            if(app->number_mode) {
                snprintf(preview, sizeof(preview), "Next: %c", (char)('0' + app->pending_digit));
            } else if(ch == ' ') {
                snprintf(preview, sizeof(preview), "Next: [SPACE]");
            } else {
                if(app->uppercase && ch >= 'a' && ch <= 'z') ch = (char)(ch - ('a' - 'A'));
                snprintf(preview, sizeof(preview), "Next: %c", ch);
            }
        } else {
            snprintf(preview, sizeof(preview), "Next: -");
        }

        canvas_draw_str(canvas, 5, 47, preview);

        if(app->number_mode) {
            canvas_draw_str(canvas, 5, 56, "DOWN: ABC  OK: Enter");
        } else {
            canvas_draw_str(canvas, 5, 56, app->uppercase ? "UP: lower  DOWN: 123" : "UP: UPPER  DOWN: 123");
        }
        canvas_draw_str(canvas, 5, 63, "OK: Enter  L: Del  R: Space");
    } else {
        char speed_text[32];
        snprintf(speed_text, sizeof(speed_text), "Mouse Speed: %d px", app->mouse_speed);
        canvas_draw_str(canvas, 5, 38, speed_text);
        canvas_draw_str(canvas, 5, 50, "Hold: Accelerates");
        canvas_draw_str(canvas, 5, 61, "L/R: Speed  MENU: Text");
    }
}


// テキスト入力補助関数（定義は後方）
static void ir_mouse_commit_pending(IRMouseApp* app);


// ========================================
// Flipper本体ボタン入力
// ========================================

static void ir_mouse_input_callback(
    InputEvent* event,
    void* context) {

    IRMouseApp* app = context;

    if(event->type != InputTypeShort) {
        return;
    }

    switch(event->key) {

    case InputKeyBack:

        if(app->text_mode) {
            ir_mouse_commit_pending(app);
            app->text_mode = false;
            FURI_LOG_I(TAG, "Returned to mouse mode");
        } else {
            FURI_LOG_I(TAG, "Exit requested");
            app->running = false;
        }

        break;

    case InputKeyLeft:

        if(app->mouse_speed > MOUSE_SPEED_MIN) {
            app->mouse_speed -= MOUSE_SPEED_STEP;
        }

        FURI_LOG_I(
            TAG,
            "Mouse speed: %d",
            app->mouse_speed
        );

        break;

    case InputKeyRight:

        if(app->mouse_speed < MOUSE_SPEED_MAX) {
            app->mouse_speed += MOUSE_SPEED_STEP;
        }

        FURI_LOG_I(
            TAG,
            "Mouse speed: %d",
            app->mouse_speed
        );

        break;

    default:
        break;
    }
}


// ========================================
// 方向コマンド判定
// ========================================

static bool ir_mouse_get_direction(
    uint32_t command,
    int8_t* dx,
    int8_t* dy,
    uint8_t* direction) {

    *dx = 0;
    *dy = 0;

    switch(command) {

    // 上
    case CMD_UP:
    case CMD_ALT_UP:

        *dy = -1;
        *direction = DIR_UP;

        return true;

    // 下
    case CMD_DOWN:
    case CMD_ALT_DOWN:

        *dy = 1;
        *direction = DIR_DOWN;

        return true;

    // 左
    case CMD_LEFT:
    case CMD_ALT_LEFT:

        *dx = -1;
        *direction = DIR_LEFT;

        return true;

    // 右
    case CMD_RIGHT:
    case CMD_ALT_RIGHT:

        *dx = 1;
        *direction = DIR_RIGHT;

        return true;

    // 右下
    case CMD_RIGHT_DOWN:

        *dx = 1;
        *dy = 1;
        *direction = DIR_RD;

        return true;

    // 右上
    case CMD_RIGHT_UP:

        *dx = 1;
        *dy = -1;
        *direction = DIR_RU;

        return true;

    // 左下
    case CMD_LEFT_DOWN:

        *dx = -1;
        *dy = 1;
        *direction = DIR_LD;

        return true;

    // 左上
    case CMD_LEFT_UP:

        *dx = -1;
        *dy = -1;
        *direction = DIR_LU;

        return true;

    default:

        *direction = DIR_NONE;

        return false;
    }
}

// ========================================
// ブラウザ戻る・進む
// ========================================

// ブラウザの戻る（Alt + Left）
static void ir_mouse_browser_back(void) {

    // HID Usage ID 0x50 = Left Arrow
    uint16_t key = 0x0050 | KEY_MOD_LEFT_ALT;

    furi_hal_hid_kb_press(key);

    furi_delay_ms(50);

    furi_hal_hid_kb_release(key);
}


// ブラウザの進む（Alt + Right）
static void ir_mouse_browser_forward(void) {

    // HID Usage ID 0x4F = Right Arrow
    uint16_t key = 0x004F | KEY_MOD_LEFT_ALT;

    furi_hal_hid_kb_press(key);

    furi_delay_ms(50);

    furi_hal_hid_kb_release(key);
}


// ========================================
// テキスト入力（英字マルチタップ）
// ========================================

static const char* const text_multitap_chars[10] = {
    " 0",       // 0
    ".,?!1",    // 1
    "abc",      // 2
    "def",      // 3
    "ghi",      // 4
    "jkl",      // 5
    "mno",      // 6
    "pqrs",     // 7
    "tuv",      // 8
    "wxyz"      // 9
};

static void ir_mouse_send_ascii(char ch) {
    uint16_t key = HID_ASCII_TO_KEY(ch);
    if(key == HID_KEYBOARD_NONE) return;

    furi_hal_hid_kb_press(key);
    furi_delay_ms(12);
    furi_hal_hid_kb_release(key);
    furi_delay_ms(12);
}

static char ir_mouse_pending_char(IRMouseApp* app) {
    const char* chars = text_multitap_chars[app->pending_digit];
    char ch = chars[app->pending_index];

    if(app->uppercase && ch >= 'a' && ch <= 'z') {
        ch = (char)(ch - ('a' - 'A'));
    }

    return ch;
}

static void ir_mouse_commit_pending(IRMouseApp* app) {
    if(!app->pending_valid) return;

    char ch = ir_mouse_pending_char(app);
    ir_mouse_send_ascii(ch);
    app->pending_valid = false;
}

static void ir_mouse_text_digit(IRMouseApp* app, uint8_t digit) {
    if(digit > 9) return;

    // 数字モードでは押した数字をそのまま送信
    if(app->number_mode) {
        ir_mouse_commit_pending(app);
        ir_mouse_send_ascii((char)('0' + digit));
        return;
    }

    uint32_t now = furi_get_tick();
    uint32_t timeout = furi_ms_to_ticks(TEXT_MULTITAP_TIMEOUT_MS);

    if(app->pending_valid &&
       app->pending_digit == digit &&
       (uint32_t)(now - app->pending_tick) <= timeout) {

        const char* chars = text_multitap_chars[digit];
        uint8_t length = 0;
        while(chars[length] != '\0') length++;

        app->pending_index = (uint8_t)((app->pending_index + 1) % length);
        app->pending_tick = now;
    } else {
        ir_mouse_commit_pending(app);
        app->pending_digit = digit;
        app->pending_index = 0;
        app->pending_tick = now;
        app->pending_valid = true;
    }
}

static void ir_mouse_text_backspace(void) {
    // HID Usage 0x2A = Keyboard Backspace
    const uint16_t key = 0x002A;
    furi_hal_hid_kb_press(key);
    furi_delay_ms(12);
    furi_hal_hid_kb_release(key);
    furi_delay_ms(12);
}

static bool ir_mouse_get_digit(uint32_t command, uint8_t* digit) {
    switch(command) {
    case CMD_NUM_0: *digit = 0; return true;
    case CMD_NUM_1: *digit = 1; return true;
    case CMD_NUM_2: *digit = 2; return true;
    case CMD_NUM_3: *digit = 3; return true;
    case CMD_NUM_4: *digit = 4; return true;
    case CMD_NUM_5: *digit = 5; return true;
    case CMD_NUM_6: *digit = 6; return true;
    case CMD_NUM_7: *digit = 7; return true;
    case CMD_NUM_8: *digit = 8; return true;
    case CMD_NUM_9: *digit = 9; return true;
    default: return false;
    }
}


// ========================================
// 赤外線受信コールバック
// ========================================

static void ir_received(
    void* context,
    InfraredWorkerSignal* signal) {

    IRMouseApp* app = context;

    if(!infrared_worker_signal_is_decoded(signal)) {
        return;
    }

    const InfraredMessage* msg =
        infrared_worker_get_decoded_signal(signal);

    if(msg == NULL) {
        return;
    }

    // NECext以外は無視
    if(msg->protocol != InfraredProtocolNECext) {
        return;
    }

    // リモコンアドレス確認
    if(msg->address != REMOTE_ADDRESS) {
        return;
    }

    // USB接続確認
    if(!furi_hal_hid_is_connected()) {
        return;
    }

    // MENUでマウスモード／テキスト入力モードを切り替え
    if(msg->command == CMD_MENU && !msg->repeat) {
        if(app->text_mode) {
            ir_mouse_commit_pending(app);
            app->text_mode = false;
            FURI_LOG_I(TAG, "Mouse mode");
        } else {
            app->pending_valid = false;
            app->uppercase = false;
            app->number_mode = false;
            app->text_mode = true;
            FURI_LOG_I(TAG, "Text input mode");
        }
        app->last_direction = DIR_NONE;
        app->accelerated_speed = app->mouse_speed;
        return;
    }

    // テキスト入力モード
    if(app->text_mode) {
        uint8_t digit = 0;

        // 数字コマンドは既存の斜め移動コマンドと値が重なるため、
        // テキストモードでは最初に数字として判定する。
        if(!msg->repeat && ir_mouse_get_digit(msg->command, &digit)) {
            ir_mouse_text_digit(app, digit);
            return;
        }

        if(msg->repeat) return;

        switch(msg->command) {
        case CMD_OK:
            ir_mouse_commit_pending(app);
            break;

        case CMD_LEFT:
            ir_mouse_commit_pending(app);
            ir_mouse_text_backspace();
            break;

        case CMD_RIGHT:
            ir_mouse_commit_pending(app);
            ir_mouse_send_ascii(' ');
            break;

        case CMD_UP:
            app->uppercase = !app->uppercase;
            break;

        case CMD_DOWN:
            ir_mouse_commit_pending(app);
            app->number_mode = !app->number_mode;
            break;

        case CMD_BROWSER_BACK:
            ir_mouse_browser_back();
            break;

        case CMD_BROWSER_FORWARD:
            ir_mouse_browser_forward();
            break;

        case CMD_RIGHT_CLICK:
            // 入力中の文字を即時確定
            ir_mouse_commit_pending(app);
            break;

        case CMD_BKSP:
            // PCへバックスペースを送信
            ir_mouse_text_backspace();
    break;

        default:
            break;
        }

        return;
    }

    // ------------------------------------
    // 方向コマンド判定
    // ------------------------------------

    int8_t dx = 0;
    int8_t dy = 0;

    uint8_t direction = DIR_NONE;

    if(ir_mouse_get_direction(
        msg->command,
        &dx,
        &dy,
        &direction)) {

        uint8_t base_speed = app->mouse_speed;

        // --------------------------------
        // 加速処理
        // --------------------------------

        if(
            msg->repeat &&
            app->last_direction == direction
        ) {

            // 同じ方向の長押しなら加速
            if(
                app->accelerated_speed <
                MOUSE_ACCEL_MAX
            ) {

                app->accelerated_speed +=
                    MOUSE_ACCEL_STEP;

                if(
                    app->accelerated_speed >
                    MOUSE_ACCEL_MAX
                ) {

                    app->accelerated_speed =
                        MOUSE_ACCEL_MAX;
                }
            }

        } else {

            // 新しい方向なら基本速度に戻す
            app->accelerated_speed = base_speed;
        }

        app->last_direction = direction;

        // --------------------------------
        // マウス移動
        // --------------------------------

        int8_t move_x =
            (int8_t)(dx * app->accelerated_speed);

        int8_t move_y =
            (int8_t)(dy * app->accelerated_speed);

        furi_hal_hid_mouse_move(
            move_x,
            move_y
        );

        return;
    }


    // ------------------------------------
    // 方向以外の操作では加速をリセット
    // ------------------------------------

    app->last_direction = DIR_NONE;

    app->accelerated_speed = app->mouse_speed;


    // ------------------------------------
    // クリック・スクロール
    // ------------------------------------

    switch(msg->command) {

    // 左クリック
    case CMD_OK:
    case CMD_LEFT_CLICK:

        // リピート信号ではクリックしない
        if(!msg->repeat) {

            furi_hal_hid_mouse_press(
                HID_MOUSE_BTN_LEFT
            );

            furi_delay_ms(40);

            furi_hal_hid_mouse_release(
                HID_MOUSE_BTN_LEFT
            );
        }

        break;


    // 右クリック
    case CMD_RIGHT_CLICK:

        // リピート信号ではクリックしない
        if(!msg->repeat) {

            furi_hal_hid_mouse_press(
                HID_MOUSE_BTN_RIGHT
            );

            furi_delay_ms(40);

            furi_hal_hid_mouse_release(
                HID_MOUSE_BTN_RIGHT
            );
        }

        break;


    // スクロール上
    case CMD_SCR_UP:

        furi_hal_hid_mouse_scroll(1);

        break;


    // スクロール下
    case CMD_SCR_DOWN:

        furi_hal_hid_mouse_scroll(-1);

        break;

    // ------------------------------------
    // ブラウザ戻る
    // ------------------------------------

    case CMD_BROWSER_BACK:

        if(!msg->repeat) {
            ir_mouse_browser_back();

            FURI_LOG_I(TAG, "Browser Back");
        }

        break;


    // ------------------------------------
    // ブラウザ進む
    // ------------------------------------

    case CMD_BROWSER_FORWARD:

        if(!msg->repeat) {
            ir_mouse_browser_forward();

            FURI_LOG_I(TAG, "Browser Forward");
        }

        break;

    default:
        break;
    }
}


// ========================================
// USB CDC復帰
// ========================================

static void ir_mouse_restore_usb(void) {

    // キーボードの押下状態を解除
    furi_hal_hid_kb_release_all();

    // マウスボタンを解放
    furi_hal_hid_mouse_release(0);

    // USB CDCへ復帰
    if(furi_hal_usb_set_config(&usb_cdc_single, NULL)) {

        FURI_LOG_I(TAG, "USB CDC restored");

    } else {

        FURI_LOG_E(TAG, "USB CDC restore failed");
    }
}


// ========================================
// メインアプリ
// ========================================

int32_t ir_mouse_app(void* p) {

    UNUSED(p);

    InfraredWorker* worker = NULL;

    Gui* gui = NULL;

    ViewPort* view_port = NULL;

    IRMouseApp app = {
        .running = true,
        .mouse_speed = MOUSE_SPEED_DEFAULT,
        .accelerated_speed = MOUSE_SPEED_DEFAULT,
        .last_direction = DIR_NONE,
        .text_mode = false,
        .pending_valid = false,
        .pending_digit = 0,
        .pending_index = 0,
        .pending_tick = 0,
        .uppercase = false,
        .number_mode = false,
    };

    FURI_LOG_I(TAG, "Starting IR Mouse v4.0");


    // ------------------------------------
    // USB HIDへ切り替え
    // ------------------------------------

    if(!furi_hal_usb_set_config(
        (FuriHalUsbInterface*)&usb_hid,
        (void*)&hid_config)) {

        FURI_LOG_E(TAG, "USB HID failed");

        ir_mouse_restore_usb();

        return 0;
    }

    // USB初期化待ち
    furi_delay_ms(3000);


    // ------------------------------------
    // GUI初期化
    // ------------------------------------

    gui = furi_record_open(RECORD_GUI);

    view_port = view_port_alloc();

    view_port_draw_callback_set(
        view_port,
        ir_mouse_draw_callback,
        &app
    );

    view_port_input_callback_set(
        view_port,
        ir_mouse_input_callback,
        &app
    );

    gui_add_view_port(
        gui,
        view_port,
        GuiLayerFullscreen
    );

    FURI_LOG_I(TAG, "GUI started");


    // ------------------------------------
    // 赤外線Worker初期化
    // ------------------------------------

    worker = infrared_worker_alloc();

    infrared_worker_rx_set_received_signal_callback(
        worker,
        ir_received,
        &app
    );

    infrared_worker_rx_enable_signal_decoding(
        worker,
        true
    );

    infrared_worker_rx_start(worker);

    FURI_LOG_I(TAG, "IR receiver started");


    // ------------------------------------
    // メインループ
    // BACKキーで終了
    // ------------------------------------

    while(app.running) {

        // マルチタップ文字は約1秒入力がなければ自動確定
        if(app.text_mode && app.pending_valid) {
            uint32_t now = furi_get_tick();
            uint32_t timeout = furi_ms_to_ticks(TEXT_MULTITAP_TIMEOUT_MS);

            if((uint32_t)(now - app.pending_tick) > timeout) {
                ir_mouse_commit_pending(&app);
            }
        }

        view_port_update(view_port);

        furi_delay_ms(50);
    }


    // ------------------------------------
    // 赤外線停止
    // ------------------------------------

    FURI_LOG_I(TAG, "Stopping IR receiver");

    infrared_worker_rx_stop(worker);

    infrared_worker_free(worker);

    worker = NULL;


    // ------------------------------------
    // GUI終了
    // ------------------------------------

    gui_remove_view_port(
        gui,
        view_port
    );

    view_port_free(view_port);

    view_port = NULL;

    furi_record_close(RECORD_GUI);

    gui = NULL;


    // ------------------------------------
    // USB CDC復帰
    // ------------------------------------

    ir_mouse_restore_usb();

    FURI_LOG_I(TAG, "IR Mouse v4.0 stopped");

    return 0;
}