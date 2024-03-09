#include "DataBalancer.h"
#include <chrono>

void DataBalancer::update(const mavlink_message_t* m, Fact* tempFact){
    uint64_t currentTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

    switch(m->msgid){
        /* TODO (at least)
         * Replace literals with macros
         * Replace message converters with the function from the mavlink files
         * error handling
         * All the other messages
         */
    case 227:{
        mavlink_cass_sensor_raw_t cs;
        mavlink_msg_cass_sensor_raw_decode(m, &cs);

        /* if first cass message, calculate the cass boot time. Do the same thing for altTime in the other cases, provided they have a timestamp at all */
        if (timeOffset == 0){
            timeOffset = currentTime - cs.time_boot_ms;
        }

        switch(cs.app_datatype){
        case 0:{ /* iMet temp */
            cass0Buf[cass0Head] = cs;
            cass0Head = (1 + cass0Head) % bufSize;
            cass0Avg = ((cass0Avg * cass0Count) + cs.values[0]) / (cass0Count + 1);

            if (cass0Count < (bufSize - 1)) cass0Count++;
            break;
        }
        case 1:{ /* RH */
            break;
        }
        case 2:{ /* temp from RH */
            break;
        }
        case 3:{ /* wind */
            break;
        }
        }
        break;
    }
    case 32:        
        break;
    case 33:
        break;
    case 137:
        break;
    case 30:
        break;
    }

    /* Some fields not yet ready... */    
    if (!(cass0Count > 0)) return;

    /* Too soon... */
    if ((currentTime - lastUpdate) < balancedDataFrequency) return;

    /* Set the data records to the averages */
    data.time = currentTime - timeOffset;
    data.iMetTemp = cass0Avg;

    /* Update facts */
    tempFact->setRawValue(cass0Avg);

    lastUpdate = currentTime;
}
