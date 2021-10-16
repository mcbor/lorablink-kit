/*
 * blink
 * Simple protocol prototype for data collection in a LoRa sensor network
 *
 * @author: Martin Bor <m.bor@lancaster.ac.uk>
 * @date: 2015-11-05  
 *
 */

#ifndef _BLINK_H_
#define _BLINK_H_

enum { MAX_BEACON_HOPS  = 5  };  // max depth of the network (keep in sync with beacon slots?)
enum { MAX_DATA_HOPS    = 5  };  // maximum number of hops for a data packet
enum { MAX_PAYLOAD_LEN  = 6  };  // bytes - maximum payload for a data packet

enum { TIME_SLOT_ms     = 5000 };  // msec - time slot length
enum { TIME_SLOTS       = 60   };  // total number of slots
enum { BEACON_SLOTS     = 5    };  // number of beacon slots
enum { DATA_SLOTS       = TIME_SLOTS - BEACON_SLOTS };  // number of data slots (time slots - beacon slots)

enum { RX_QUEUE_DEPTH   = 1 };  // maximum number of packets in the rx queue
enum { TX_QUEUE_DEPTH   = 1 };  // maximum number of packets in the tx queue

enum { CAD_CHECKS         = 3   }; // number of CAD checks to run
enum { MAX_MISSED_BEACONS = BEACON_SLOTS * 3   }; // maximum number of missed beacon rounds
enum { MAX_DRIFT_ms       = 400 }; //  msec - maximum drift between wakeup slots and beacons

enum { AIRTIME_BEACON_us  = 827392 }; // airtime for the beacon (assuming SF12/BW125,CR_4_5,CRC,HDR and 4 bytes)

#if !defined(BLINK_USE_CAD)
#define BLINK_USE_CAD       FALSE    // don't use CAD by default
#endif

#define AIRTIME_BEACON_ticks us2osticks(AIRTIME_BEACON_us)
#define TIME_SLOT_ticks      ms2osticks(TIME_SLOT_ms)

enum _event_t {
  EVENT_SYNC = 1,        // got sync
  EVENT_LOST_SYNC,       // lost sync
  EVENT_RXCOMPLETE,      // received data ready
  EVENT_TXCOMPLETE       // transmit data done
};
typedef enum _event_t event_t;

enum _packet_type_t {
  BEACON = 0x00,
  DATA   = 0x01,
};
typedef enum _packet_type_t packet_type_t;

/* packet definitions */
struct _header_t {
  packet_type_t type : 4;
  u1_t          hop  : 4;
  u1_t          dest;
} __attribute__((packed));
typedef struct _header_t header_t;

struct _footer_t {
 u2_t           trace; 
} __attribute__((packed));
typedef struct _footer_t footer_t;

struct _beacon_msg_t {
  header_t      header;
  // empty
  footer_t      footer;
} __attribute__((packed));
typedef struct _beacon_msg_t beacon_msg_t;

struct _data_msg_t {
  header_t      header;
  u1_t          payload[MAX_PAYLOAD_LEN];
  footer_t      footer;
} __attribute__((packed));
typedef struct _data_msg_t data_msg_t;

enum {
  DEST_ROOT      = 0x00,
  DEST_BROADCAST = 0xff,
};

enum {
  PEND_NONE      = 0x00,
  PEND_BEACON_TX = 0x01,
  PEND_DATA_TX   = 0x02,
  PEND_DATA_RX   = 0x04,
};

/* Operating modes */
enum { OP_NONE   = 0x0000,
       OP_READY  = 0x0001, // ready
       OP_SCAN   = 0x0002, // scan for beacons
       OP_TRACK  = 0x0004, // tracking a beacon
       OP_TXBCN  = 0x0008, // TX beacon
       OP_TXDATA = 0x0010, // TX user data
       OP_RXBCN  = 0x0020, // RX beacon
       OP_RXDATA = 0x0040, // RX user data
       OP_ROOT   = 0x0080, // Root node
       OP_NODE   = 0x0100, // Regular node
};

/* blink control struct */
struct blink_t {
  u2_t opmode;        // current operating mode
  u1_t slot;          // current time slot
  u1_t hop;           // our hop (distance) to the sink
  u1_t pending;       // pending bits
  u1_t nodeid;        // id of this node
  u1_t missed_beacons;// number of missed beacons
  u1_t hop_updated;   // 0 if hop wasn't updated this epoch, 1 otherwise
};
extern struct blink_t BLINK;

/* exported function prototypes */
extern void on_event(event_t ev);

void blink_init(void);
void blink_reset(void);
void blink_start_sync(void);
void blink_tx(u1_t *buffer, size_t n);
void blink_rx(u1_t *buffer, size_t n);

#define TRACE_MASK  (0x7)
#define TRACE_SHIFT (3)
#define TRACE_MAX   (16 / TRACE_SHIFT)

#define ROOT_ID (0)

#endif /* end of include guard: _BLINK_H_ */
