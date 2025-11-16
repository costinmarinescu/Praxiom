#pragma once

#include <cstdint>

#define min
#define max
#include <host/ble_gap.h>
#undef max
#undef min

namespace Pinetime {
  namespace Controllers {
    class PraxiomController;
    
    class PraxiomService {
    public:
      explicit PraxiomService(PraxiomController& praxiomController);
      void Init();
      
      int OnBioAgeCommand(struct ble_gatt_access_ctxt* ctxt);
      
    private:
      PraxiomController& praxiomController;
      
      static constexpr uint16_t praxiomServiceId {0x1900};
      static constexpr uint16_t bioAgeCharId {0x1901};
      
      static constexpr ble_uuid128_t praxiomServiceUuid {
        .u {.type = BLE_UUID_TYPE_128},
        .value = {0xd0, 0x42, 0x19, 0x3a, 0x3b, 0x43, 0x23, 0x8e,
                  0xfe, 0x48, 0xfc, 0x78, 0x00, 0x19, 0x00, 0x00}
      };
      
      static constexpr ble_uuid128_t bioAgeCharUuid {
        .u {.type = BLE_UUID_TYPE_128},
        .value = {0xd0, 0x42, 0x19, 0x3a, 0x3b, 0x43, 0x23, 0x8e,
                  0xfe, 0x48, 0xfc, 0x78, 0x01, 0x19, 0x00, 0x00}
      };
      
      struct ble_gatt_chr_def characteristicDefinition[2];
      struct ble_gatt_svc_def serviceDefinition[2];
      
      uint16_t bioAgeHandle;
    };
  }
}
