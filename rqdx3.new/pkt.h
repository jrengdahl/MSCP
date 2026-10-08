/*
 *  file = PKT.H
 *  project = RQDX3
 *  author = Stephen F. Shirron
 *
 *  definition of $PKT structure
 *
 *  link		-- link to next PKT in chain
 *  size		-- size of data portion of packet in bytes
 *  type		-- type of packet (sequential message, datagram)
 *  connid		-- connection ID of packet recipient
 *  data[60]		-- actual data of packet
 */

struct $pkt
    {
    struct $pkt		*link;
    word		size;
    byte		type;
    byte		connid;
    byte		data[60];
    };
