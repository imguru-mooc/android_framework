// IMyDataCallback.h

#include <binder/IInterface.h>
#include <binder/Parcel.h>

namespace android {

	class IMyDataCallback : public IInterface {
		public:
			DECLARE_META_INTERFACE(MyDataCallback);

			virtual void onDataReceived(const String16& data) = 0;
	};

	class BnMyDataCallback : public BnInterface<IMyDataCallback> {
		public:
			status_t onTransact(uint32_t code, const Parcel& data,
					Parcel* reply, uint32_t flags = 0) override;
	};

} // namespace android

