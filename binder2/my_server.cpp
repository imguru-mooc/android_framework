#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <grp.h>

#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>
#include <utils/Log.h>

using namespace android;

namespace android {
	class AAA : public BBinder {
		public:
			void ledOn(void) {
				printf("AAA::ledOn()\n");
			}

			status_t onTransact( uint32_t code, const Parcel& data, Parcel* reply, uint32_t flags) {
				(void)data;
				(void)reply;
				(void)flags;
				switch (code) {
					case 1:
						ledOn(); break;
					default:
					   return UNKNOWN_TRANSACTION;
				}
				return NO_ERROR;
			}
	};
};

int main()
{
    sp<ProcessState> proc(ProcessState::self());
    sp<IServiceManager> sm = defaultServiceManager();
	sm->addService( String16("led.service"), new AAA);
    IPCThreadState::self()->joinThreadPool();
	return 0;
}




