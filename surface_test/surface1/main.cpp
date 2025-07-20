#include <iostream>
#include <unistd.h>

#include <binder/ProcessState.h>
#include <gui/SurfaceComposerClient.h>
#include <gui/Surface.h>
#include <android/native_window.h>

// 디스플레이 정보를 가져오던 헤더들을 제거했습니다.
// #include <ui/DisplayState.h>
// #include <ui/ISurfaceComposer.h>

using namespace android;

int main(int , char** ) {
    // 네이티브 프로세스 및 바인더 스레드 풀 초기화
    sp<ProcessState> proc(ProcessState::self());
    ProcessState::self()->startThreadPool();

    // SurfaceComposerClient를 통해 SurfaceFlinger에 연결
    sp<SurfaceComposerClient> client = new SurfaceComposerClient();
    if (client->initCheck() != NO_ERROR) {
        std::cerr << "Failed to connect to SurfaceFlinger" << std::endl;
        return -1;
    }

    // ------------------------------------------------------------------------
    // 오류가 발생한 디스플레이 정보 조회 부분을 모두 삭제했습니다.
    // 이 부분은 서페이스를 생성하고 표시하는 핵심 기능에 필수가 아닙니다.
    // ------------------------------------------------------------------------

    std::cout << "Creating surface..." << std::endl;

    // 서페이스(레이어) 생성
    sp<SurfaceControl> surfaceControl = client->createSurface(
            String8("My Cpp Surface"), // 서페이스 이름
            400,                       // 너비
            400,                       // 높이
            PIXEL_FORMAT_RGBA_8888,    // 픽셀 포맷
            0);

    if (surfaceControl == nullptr || !surfaceControl->isValid()) {
        std::cerr << "Failed to create surface" << std::endl;
        return -1;
    }

    // 서페이스 속성 설정
    // 트랜잭션을 통해 서페이스의 Z-오더, 위치, 가시성 등을 설정합니다.
    SurfaceComposerClient::Transaction tx;
    tx.setLayer(surfaceControl, 0x7FFFFFFF)      // Z-오더를 최상위로 설정
      .setPosition(surfaceControl, 100, 100)     // 화면 내 (100, 100) 위치에 배치
      .show(surfaceControl)                      // 서페이스를 보이도록 설정
      .apply();                                  // 트랜잭션 적용

    // ANativeWindow를 통해 서페이스 버퍼에 접근
    sp<ANativeWindow> window = surfaceControl->getSurface();
    ANativeWindow_Buffer buffer;

    if (ANativeWindow_lock(window.get(), &buffer, nullptr) < 0) {
        std::cerr << "Failed to lock window buffer" << std::endl;
        return -1;
    }

    // 버퍼를 붉은색(RGBA)으로 채우기
    uint8_t* pixels = reinterpret_cast<uint8_t*>(buffer.bits);
    for (int y = 0; y < buffer.height; ++y) {
        for (int x = 0; x < buffer.width; ++x) {
            // 픽셀 데이터는 한 줄(stride)의 너비에 맞춰 저장됩니다.
            uint8_t* pixel = pixels + (y * buffer.stride + x) * 4;
            pixel[0] = 255; // R
            pixel[1] = 0;   // G
            pixel[2] = 0;   // B
            pixel[3] = 255; // A (불투명)
        }
    }

    // 버퍼를 unlock하고 화면에 표시 (post)
    if (ANativeWindow_unlockAndPost(window.get()) < 0) {
        std::cerr << "Failed to unlock and post buffer" << std::endl;
        return -1;
    }

    std::cout << "Surface drawn. Press Ctrl+C to exit." << std::endl;

    // 애플리케이션이 바로 종료되지 않도록 대기
    while (true) {
        sleep(1);
    }

    return 0;
}
