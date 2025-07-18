// IMyDataService.h

#include <binder/IInterface.h>
#include "IMyDataCallback.h"

namespace android {

	class IMyDataService : public IInterface {
		public:
			DECLARE_META_INTERFACE(MyDataService);

			virtual void requestData(const sp<IMyDataCallback>& callback) = 0;
	};

	class BnMyDataService : public BnInterface<IMyDataService> {
		public:
			status_t onTransact(uint32_t code, const Parcel& data,
					Parcel* reply, uint32_t flags = 0) override;
	};

}

