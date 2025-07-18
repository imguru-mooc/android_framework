// IMyDataCallback.cpp

#define DO_NOT_CHECK_MANUAL_BINDER_INTERFACES
#include "IMyDataCallback.h"

namespace android {

	enum {
		ON_DATA_RECEIVED = IBinder::FIRST_CALL_TRANSACTION,
	};

	IMPLEMENT_META_INTERFACE(MyDataCallback, "com.example.IMyDataCallback");

	status_t BnMyDataCallback::onTransact(uint32_t code, const Parcel& data,
			Parcel* reply, uint32_t flags) {
		switch (code) {
			case ON_DATA_RECEIVED: {
									   String16 result = data.readString16();
									   onDataReceived(result);
									   return NO_ERROR;
								   }
			default:
								   return BBinder::onTransact(code, data, reply, flags);
		}
	}

}

