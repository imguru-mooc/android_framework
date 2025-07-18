// IMyDataService.cpp
#define DO_NOT_CHECK_MANUAL_BINDER_INTERFACES
#include "IMyDataService.h"

namespace android {

	enum {
		REQUEST_DATA = IBinder::FIRST_CALL_TRANSACTION,
	};

	IMPLEMENT_META_INTERFACE(MyDataService, "com.example.IMyDataService");

	status_t BnMyDataService::onTransact(uint32_t code, const Parcel& data,
			Parcel* reply, uint32_t flags) {
		switch (code) {
			case REQUEST_DATA: {
								   sp<IBinder> cbBinder = data.readStrongBinder();
								   sp<IMyDataCallback> callback = interface_cast<IMyDataCallback>(cbBinder);
								   requestData(callback);
								   return NO_ERROR;
							   }
			default:
							   return BBinder::onTransact(code, data, reply, flags);
		}
	}

}

