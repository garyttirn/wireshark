/** @file
 *
 * Declarations of our own routines for writing pcap and pcapng files.
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 1998 Gerald Combs
 *
 * Derived from code in the Wiretap Library
 * Copyright (c) 1998 by Gilbert Ramirez <gram@alumni.rice.edu>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <wiretap/wtap.h>

typedef struct pcapio_writer pcapio_writer;

extern pcapio_writer*
writecap_fopen(const char *filename, wtap_compression_type ctype, int *err);

extern pcapio_writer*
writecap_fdopen(int fd, wtap_compression_type ctype, int *err);

extern pcapio_writer*
writecap_open_stdout(wtap_compression_type ctype, int *err);

extern bool
writecap_flush(pcapio_writer* pfile, int *err);

/* Close open file handles and frees memory associated with pfile.
 *
 * Return true on success, returns false and sets err (optional) on failure.
 * err can be NULL, e.g. if closing after some other failure that is more
 * relevant to report, or when exiting a program. */
extern bool
writecap_close(pcapio_writer* pfile, int *err);

/* Writing pcap files */

/** Write the file header to a dump file.
   Returns true on success, false on failure.
   Sets "*err" to an error code, or 0 for a short write, on failure*/
extern bool
libpcap_write_file_header(pcapio_writer* pfile, int linktype, int snaplen,
                          bool ts_nsecs, uint64_t *bytes_written, int *err);

/** Write a record for a packet to a dump file.
   Returns true on success, false on failure. */
extern bool
libpcap_write_packet(pcapio_writer* pfile,
                     time_t sec, uint32_t usec,
                     uint32_t caplen, uint32_t len,
                     const uint8_t *pd,
                     uint64_t *bytes_written, int *err);

/* Writing pcapng files */

/* Write a pre-formatted pcapng block */
extern bool
pcapng_write_block(pcapio_writer* pfile,
                   const uint8_t *data,
                   uint32_t block_total_length,
                   uint64_t *bytes_written,
                   int *err);

/** Write a section header block (SHB)
 *
 */
extern bool
pcapng_write_section_header_block(pcapio_writer* pfile,  /**< Write information */
                                  GPtrArray *comments,  /**< Comments on the section, Optinon 1 opt_comment
                                                         * UTF-8 strings containing comments that areassociated to the current block.
                                                         */
                                  const char *hw,       /**< HW, Optinon 2 shb_hardware
                                                         * An UTF-8 string containing the description of the hardware  used to create this section.
                                                         */
                                  const char *os,       /**< Operating system name, Optinon 3 shb_os
                                                         * An UTF-8 string containing the name of the operating system used to create this section.
                                                         */
                                  const char *appname,  /**< Application name, Optinon 4 shb_userappl
                                                         * An UTF-8 string containing the name of the application  used to create this section.
                                                         */
                                  uint64_t section_length, /**< Length of section */
                                  uint64_t *bytes_written, /**< Number of written bytes */
                                  int *err /**< Error type */
                                  );

extern bool
pcapng_write_interface_description_block(pcapio_writer* pfile,
                                         const char *comment,  /* OPT_COMMENT           1 */
                                         const char *name,     /* IDB_NAME              2 */
                                         const char *descr,    /* IDB_DESCRIPTION       3 */
                                         const char *filter,   /* IDB_FILTER           11 */
                                         const char *os,       /* IDB_OS               12 */
                                         const char *hardware, /* IDB_HARDWARE         15 */
                                         int link_type,
                                         int snap_len,
                                         uint64_t *bytes_written,
                                         uint64_t if_speed,     /* IDB_IF_SPEED          8 */
                                         uint8_t tsresol,       /* IDB_TSRESOL           9 */
                                         int *err);

extern bool
pcapng_write_interface_statistics_block(pcapio_writer* pfile,
                                        uint32_t interface_id,
                                        uint64_t *bytes_written,
                                        const char *comment,   /* OPT_COMMENT           1 */
                                        uint64_t isb_starttime, /* ISB_STARTTIME         2 */
                                        uint64_t isb_endtime,   /* ISB_ENDTIME           3 */
                                        uint64_t isb_ifrecv,    /* ISB_IFRECV            4 */
                                        uint64_t isb_ifdrop,    /* ISB_IFDROP            5 */
                                        int *err);

extern bool
pcapng_write_enhanced_packet_block(pcapio_writer* pfile,
                                   const char *comment,
                                   time_t sec, uint32_t usec,
                                   uint32_t caplen, uint32_t len,
                                   uint32_t interface_id,
                                   unsigned ts_mul,
                                   const uint8_t *pd,
                                   uint32_t flags,
                                   uint64_t *bytes_written,
                                   int *err);
