# Protocol

All messages start with a header:
- `type` (1 byte, MessageType)
- `length` (2 bytes, payload size, not including header)

---

| Message Type                | Payload Fields                                                                                   |
|----------------------------|-------------------------------------------------------------------------------------------------|
| **MSG_PUT**                | `keyLen` (2), `key` (keyLen), `valueLen` (2), `value` (valueLen)                                 |
| **MSG_PUT_RESPONSE**       | `result` (1), `keyLen` (2), `key` (keyLen)                                                      |
| **MSG_GET**                | `keyLen` (2), `key` (keyLen)                                                                    |
| **MSG_GET_RESPONSE**       | `result` (1), `keyLen` (2), `key` (keyLen), `valueLen` (2), `value` (valueLen, if > 0)          |
| **MSG_STORE_REQUEST**      | `clientId` (4), `keyLen` (2), `key` (keyLen), `valueLen` (2), `value` (valueLen)                |
| **MSG_STORE_RESPONSE**     | `result` (1), `clientId` (4), `keyLen` (2), `key` (keyLen)                                      |
| **MSG_RETRIEVE_REQUEST**   | `clientId` (4), `keyLen` (2), `key` (keyLen)                                                    |
| **MSG_RETRIEVE_RESPONSE**  | `result` (1), `clientId` (4), `keyLen` (2), `key` (keyLen), `valueLen` (2), `value` (valueLen, if > 0) |
| **MSG_WORKER_REGISTRY_START** | `totalWorkers` (4)                                                                           |
| **MSG_WORKER_ENTRY**       | `workerId` (4), `addrLen` (2), `address` (addrLen), `port` (2), `shouldExportData` (1)          |
| **MSG_WORKER_REGISTRY_END**| (no payload)                                                                                      |
| **MSG_DATA_EXPORT_START**  | `totalEntries` (4)                                                                              |
| **MSG_DATA_ENTRY**         | `keyLen` (2), `key` (keyLen), `valueLen` (2), `value` (valueLen)                                |
| **MSG_DATA_EXPORT_END**    | (no payload)                                                                                      |
| **MSG_WORKER_READY**       | `workerId` (4), `peerPort` (2)                                                                  |
| **MSG_WORKER_NOT_READY**   | `workerId` (4)                                                                                  |
| **MSG_PEER_NOTIFY**        | `keyLen` (2), `key` (keyLen), `valueLen` (2), `value` (valueLen)                                |
| **MSG_SHUTDOWN**           | (no payload)                                                                                      |
| **MSG_ERROR**              | `errorCode` (1), `messageLen` (2), `message` (messageLen, if > 0)                               |

