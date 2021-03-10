#include "rtl-sdr_export.h"
#include "rtl_rec.h"

#ifdef __cplusplus
extern "C" {
#endif

__SDR_EXPORT void SetNoiseState(rtlsdr_dev_t *dev, uint8_t state);
__SDR_EXPORT void InitializeSynchronization();

#ifdef __cplusplus
}
#endif