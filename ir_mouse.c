#include <furi.h>
#include <furi_hal_usb.h>
#include <furi_hal_usb_hid.h>
#include <infrared_worker.h>

#include <gui/gui.h>
#include <gui/view_port.h>
#include <gui/canvas.h>
#include <input/input.h>

#define TAG "IR_Mouse"

// リモコンのアドレス
#define REMOTE_ADDRESS 0x00007201

// NECext コマンド
#define CMD_OK     0x0000A05F
#define CMD_UP     0x0000FC03
#define CMD_DOWN   0x0000A15E
#define CMD_RIGHT  0x0000BF40
#define CMD_LEFT   0x0000FD02

// マウス速度
#define MOUSE_SPEED_MIN 4
#define MOUSE_SPEED_MAX 24
#define MOUSE_SPEED_STEP 4
#define MOUSE_SPEED_DEFAULT 12

// アプリ状態
typedef struct {
    volatile bool running;
    volatile uint8_t mouse_speed;
} IRMouseApp;

// USB HID設定
static const FuriHalUsbHidConfig hid_config = {
    .vid = 0x0483,
    .pid = 0x5740,
    .manuf = "Flipper Devices Inc.",
    .product = "Flipper IR Mouse",
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
    canvas_draw_str(canvas, 12, 12, "IR Mouse Active");

    canvas_set_font(canvas, FontSecondary);

    canvas_draw_str(canvas, 5, 25, "USB: HID Connected");

    char speed_text[32];

    snprintf(
        speed_text,
        sizeof(speed_text),
        "Mouse Speed: %d px",
        app->mouse_speed
    );

    canvas_draw_str(canvas, 5, 39, speed_text);

    canvas_draw_str(canvas, 5, 51, "LEFT: Slower");

    canvas_draw_str(canvas, 5, 62, "RIGHT: Faster");

    // BACKキーは終了操作
}


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

        FURI_LOG_I(TAG, "Exit requested");

        app->running = false;

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

    // アドレス確認
    if(msg->address != REMOTE_ADDRESS) {
        return;
    }

    // USB接続確認
    if(!furi_hal_hid_is_connected()) {
        return;
    }

    // 現在のマウス速度
    uint8_t speed = app->mouse_speed;

    switch(msg->command) {

    case CMD_UP:

        furi_hal_hid_mouse_move(0, -(int8_t)speed);

        break;

    case CMD_DOWN:

        furi_hal_hid_mouse_move(0, (int8_t)speed);

        break;

    case CMD_LEFT:

        furi_hal_hid_mouse_move(-(int8_t)speed, 0);

        break;

    case CMD_RIGHT:

        furi_hal_hid_mouse_move((int8_t)speed, 0);

        break;

    case CMD_OK:

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

    default:
        break;
    }
}


// ========================================
// USB CDC復帰
// ========================================

static void ir_mouse_restore_usb(void) {

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
    };

    FURI_LOG_I(TAG, "Starting IR Mouse v2.0");


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

        // 画面更新
        view_port_update(view_port);

        // CPU負荷を抑える
        furi_delay_ms(100);
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

    FURI_LOG_I(TAG, "IR Mouse stopped");

    return 0;
}