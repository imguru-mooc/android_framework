// SPDX-License-Identifier: Apache-2.0
#define LOG_TAG "sf_vsync_anim_final"

#include <iostream>
#include <unistd.h>
#include <memory>                              // ⭐️ unique_ptr
#include <gui/SurfaceComposerClient.h>
#include <gui/Surface.h>
#include <gui/DisplayEventReceiver.h>
#include <ui/DynamicDisplayInfo.h>
#include <utils/Looper.h>
#include <utils/Trace.h>
#include <log/log.h>

using namespace android;

class VSyncHandler : public LooperCallback {
	public:
		VSyncHandler(SurfaceComposerClient::Transaction& tx,
				const sp<SurfaceControl>& sc,
				DisplayEventReceiver* recv,   // ⭐️ raw pointer
				int32_t screenW)
			: mSurface(sc), mReceiver(recv), mScreenW(screenW), mTx(tx) {}

		int handleEvent(int /*fd*/, int /*events*/, void* /*data*/) override {
			std::cout << "handleEvent()" << std::endl;
			DisplayEventReceiver::Event ev;
			while (mReceiver->getEvents(&ev, 1) == 1) {
				if (ev.header.type == DisplayEventReceiver::DISPLAY_EVENT_VSYNC) {
					step();
					mReceiver->requestNextVsync();
				}
			}
			return 1;
		}

	private:
		void step() {
			if (mSurface == nullptr || !mSurface->isValid()) {
				ALOGE("step: invalid SurfaceControl");
				return;
			}
			std::cout << "step()\n" << std::endl;
			std::cout << "mSurface : " << mSurface.get() << std::endl;
			mX = (mX + 4) % mScreenW;

			mTx.setPosition(mSurface, mX, 200)
			   .apply(false);                                  // 트랜잭션 적용
		}
		sp<SurfaceControl>   mSurface;
		DisplayEventReceiver* mReceiver;   // ⭐️ non‑RefBase
		int32_t              mScreenW;
		int32_t              mX = 0;
		SurfaceComposerClient::Transaction& mTx;
};

int main() {
	ALOGI("sf_vsync_anim (final) start");

	auto ids = SurfaceComposerClient::getPhysicalDisplayIds();
	if (ids.empty()) {
		ALOGE("no physical displays!");
		return -1;
	}
	ui::DynamicDisplayInfo ddi;
	SurfaceComposerClient::getDynamicDisplayInfoFromId(ids.front().value, &ddi);
	const int32_t screenW = ddi.getActiveDisplayMode()->resolution.getWidth();

	std::cerr << "scrrenW : " << screenW << std::endl;

	sp<SurfaceComposerClient> client = new SurfaceComposerClient();
	if (client->initCheck() != NO_ERROR) {
		std::cerr << "Failed to connect to SurfaceFlinger" << std::endl;
		return -1;
	}

	sp<SurfaceControl> sc = client->createSurface(
			String8("vsync_rect"), 128, 128,
			PIXEL_FORMAT_RGBA_8888, 0);

	if (sc == nullptr || !sc->isValid()) {
		std::cerr << "Failed to create surface" << std::endl;
		return -1;
	}

	/*
	SurfaceComposerClient::Transaction tx;
	tx.setLayer(sc, 0x7FFFFFFF)      // Z-오더를 최상위로 설정
		.show(sc)                      // 서페이스를 보이도록 설정
		.apply();                                  // 트랜잭션 적용
		*/

	auto tx = std::make_unique<SurfaceComposerClient::Transaction>();
	tx->setLayer(sc, 0x7FFFFFFF)      // Z-오더를 최상위로 설정
		.show(sc)                      // 서페이스를 보이도록 설정
		.apply();                                  // 트랜잭션 적용

	// ANativeWindow를 통해 서페이스 버퍼에 접근
	sp<ANativeWindow> surf = sc->getSurface();
	ANativeWindow_Buffer buf;

	if (ANativeWindow_lock(surf.get(), &buf, nullptr) < 0) {
		std::cerr << "Failed to lock window buffer" << std::endl;
		return -1;
	}


	uint32_t* p = static_cast<uint32_t*>(buf.bits);
	for (int y = 0; y < buf.height; ++y)
		for (int x = 0; x < buf.width; ++x)
			p[y * buf.stride + x] = 0xFF00FF00;

	// 버퍼를 unlock하고 화면에 표시 (post)
	if (ANativeWindow_unlockAndPost(surf.get()) < 0) {
		std::cerr << "Failed to unlock and post buffer" << std::endl;
		return -1;
	}

	std::cout << "ANativeWindow_unlockAndPost after" << std::endl;

	/*
	while (true) {
		sleep(1);
	}
	*/


	std::unique_ptr<DisplayEventReceiver> recv = std::make_unique<DisplayEventReceiver>();
	if (recv->initCheck() != NO_ERROR) {
		ALOGE("DisplayEventReceiver init failed");
		return -1;
	}
	recv->setVsyncRate(1);
	recv->requestNextVsync();

	sp<Looper> looper = Looper::prepare(0);
	std::cout << "sc : " << sc.get() << std::endl;
	looper->addFd(recv->getFd(), 0,
			Looper::EVENT_INPUT,
			new VSyncHandler(*tx, sc, recv.get(), screenW),
			nullptr);

	const nsecs_t until = systemTime() + seconds_to_nanoseconds(10);
	while (systemTime() < until) 
		looper->pollAll(-1);
	return 0;
}

