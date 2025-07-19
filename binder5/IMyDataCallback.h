// IMyDataCallback.cpp

#ifndef ANDROID_IMYDATACALLBACK_SERVICE_H
#define ANDROID_IMYDATACALLBACK_SERVICE_H

#define DO_NOT_CHECK_MANUAL_BINDER_INTERFACES

#include <binder/IInterface.h>
#include <binder/Parcel.h>

namespace android {

	class IMyDataCallback : public IInterface {
		public:
			DECLARE_META_INTERFACE(MyDataCallback);

			virtual void onDataReceived(const String16& data) = 0;
			enum {
				ON_DATA_RECEIVED = IBinder::FIRST_CALL_TRANSACTION,
			};

	};

	class BnMyDataCallback : public BnInterface<IMyDataCallback> {
		public:
			status_t onTransact(uint32_t code, const Parcel& data,
					Parcel* reply, uint32_t flags = 0) override;
	};

} // namespace android

#endif
