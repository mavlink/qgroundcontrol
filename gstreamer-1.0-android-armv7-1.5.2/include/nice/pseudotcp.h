/*
 * This file is part of the Nice GLib ICE library.
 *
 * (C) 2010, 2014 Collabora Ltd.
 *  Contact: Philip Withnall

 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is the Nice GLib ICE library.
 *
 * The Initial Developers of the Original Code are Collabora Ltd and Nokia
 * Corporation. All Rights Reserved.
 *
 * Contributors:
 *   Youness Alaoui, Collabora Ltd.
 *   Philip Withnall, Collabora Ltd.
 *
 * Alternatively, the contents of this file may be used under the terms of the
 * the GNU Lesser General Public License Version 2.1 (the "LGPL"), in which
 * case the provisions of LGPL are applicable instead of those above. If you
 * wish to allow use of your version of this file only under the terms of the
 * LGPL and not to allow others to use your version of this file under the
 * MPL, indicate your decision by deleting the provisions above and replace
 * them with the notice and other provisions required by the LGPL. If you do
 * not delete the provisions above, a recipient may use your version of this
 * file under either the MPL or the LGPL.
 */

#ifndef __LIBNICE_PSEUDOTCP_H__
#define __LIBNICE_PSEUDOTCP_H__

/**
 * SECTION:pseudotcp
 * @short_description: Pseudo TCP implementation
 * @include: pseudotcp.h
 * @stability: Stable
 *
 * The #PseudoTcpSocket is an object implementing a Pseudo Tcp Socket for use
 * over UDP.
 * The socket will implement a subset of the TCP stack to allow for a reliable
 * transport over non-reliable sockets (such as UDP).
 *
 * See the file tests/test-pseudotcp.c in the source package for an example
 * of how to use the object.
 *
 * Since: 0.0.11
 */



#include <glib-object.h>

#ifndef __GTK_DOC_IGNORE__
#ifdef G_OS_WIN32
#  include <winsock2.h>
#  define ECONNABORTED WSAECONNABORTED
#  define ENOTCONN WSAENOTCONN
#  define EWOULDBLOCK WSAEWOULDBLOCK
#  define ECONNRESET WSAECONNRESET
#endif
#endif

#include "agent.h"

G_BEGIN_DECLS

/**
 * PseudoTcpSocket:
 *
 * The #PseudoTcpSocket is the GObject implementing the Pseudo TCP Socket
 *
 * Since: 0.0.11
 */
typedef struct _PseudoTcpSocket PseudoTcpSocket;

typedef struct _PseudoTcpSocketClass PseudoTcpSocketClass;

GType pseudo_tcp_socket_get_type (void);

/* TYPE MACROS */
#define PSEUDO_TCP_SOCKET_TYPE \
  (pseudo_tcp_socket_get_type ())
#define PSEUDO_TCP_SOCKET(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), PSEUDO_TCP_SOCKET_TYPE, \
                              PseudoTcpSocket))
#define PSEUDO_TCP_SOCKET_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass), PSEUDO_TCP_SOCKET_TYPE, \
                           PseudoTcpSocketClass))
#define IS_PSEUDO_TCP_SOCKET(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj), PSEUDO_TCP_SOCKET_TYPE))
#define IS_PSEUDO_TCP_SOCKET_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass), PSEUDO_TCP_SOCKET_TYPE))
#define PSEUDOTCP_SOCKET_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), PSEUDO_TCP_SOCKET_TYPE, \
                              PseudoTcpSocketClass))

struct _PseudoTcpSocketClass {
    GObjectClass parent_class;
};

typedef struct _PseudoTcpSocketPrivate PseudoTcpSocketPrivate;

struct _PseudoTcpSocket {
    GObject parent;
    PseudoTcpSocketPrivate *priv;
};

/**
 * PseudoTcpDebugLevel:
 * @PSEUDO_TCP_DEBUG_NONE: Disable debug messages
 * @PSEUDO_TCP_DEBUG_NORMAL: Enable basic debug messages
 * @PSEUDO_TCP_DEBUG_VERBOSE: Enable verbose debug messages
 *
 * Valid values of debug levels to be set.
 *
 * Since: 0.0.11
 */
typedef enum {
  PSEUDO_TCP_DEBUG_NONE = 0,
  PSEUDO_TCP_DEBUG_NORMAL,
  PSEUDO_TCP_DEBUG_VERBOSE,
} PseudoTcpDebugLevel;

/**
 * PseudoTcpState:
 * @TCP_LISTEN: The socket's initial state. The socket isn't connected and is
 * listening for an incoming connection
 * @TCP_SYN_SENT: The socket has sent a connection request (SYN) packet and is
 * waiting for an answer
 * @TCP_SYN_RECEIVED: The socket has received a connection request (SYN) packet.
 * @TCP_ESTABLISHED: The socket is connected
 * @TCP_CLOSED: The socket has been closed
 * @TCP_FIN_WAIT_1: The socket has been closed locally but not remotely
 * (Since: 0.1.8)
 * @TCP_FIN_WAIT_2: The socket has been closed locally but not remotely
 * (Since: 0.1.8)
 * @TCP_CLOSING: The socket has been closed locally and remotely
 * (Since: 0.1.8)
 * @TCP_TIME_WAIT: The socket has been closed locally and remotely
 * (Since: 0.1.8)
 * @TCP_CLOSE_WAIT: The socket has been closed remotely but not locally
 * (Since: 0.1.8)
 * @TCP_LAST_ACK: The socket has been closed locally and remotely
 * (Since: 0.1.8)
 *
 * An enum representing the state of the #PseudoTcpSocket. These states
 * correspond to the TCP states in RFC 793.
 * <para> See also: #PseudoTcpSocket:state </para>
 *
 * Since: 0.0.11
 */
typedef enum {
  TCP_LISTEN,
  TCP_SYN_SENT,
  TCP_SYN_RECEIVED,
  TCP_ESTABLISHED,
  TCP_CLOSED,
  TCP_FIN_WAIT_1,
  TCP_FIN_WAIT_2,
  TCP_CLOSING,
  TCP_TIME_WAIT,
  TCP_CLOSE_WAIT,
  TCP_LAST_ACK,
} PseudoTcpState;

/**
 * PseudoTcpWriteResult:
 * @WR_SUCCESS: The write operation was successful
 * @WR_TOO_LARGE: The socket type requires that message be sent atomically
 * and the size of the message to be sent made this impossible.
 * @WR_FAIL: There was an error sending the message
 *
 * An enum representing the result value of the write operation requested by
 * the #PseudoTcpSocket.
 * <para> See also: %PseudoTcpCallbacks:WritePacket </para>
 *
 * Since: 0.0.11
 */
typedef enum {
  WR_SUCCESS,
  WR_TOO_LARGE,
  WR_FAIL
} PseudoTcpWriteResult;

/**
 * PseudoTcpShutdown:
 * @PSEUDO_TCP_SHUTDOWN_RD: Shut down the local reader only
 * @PSEUDO_TCP_SHUTDOWN_WR: Shut down the local writer only
 * @PSEUDO_TCP_SHUTDOWN_RDWR: Shut down both reading and writing
 *
 * Options for which parts of a connection to shut down when calling
 * pseudo_tcp_socket_shutdown(). These correspond to the values passed to POSIX
 * shutdown().
 *
 * Since: 0.1.8
 */
typedef enum {
  PSEUDO_TCP_SHUTDOWN_RD,
  PSEUDO_TCP_SHUTDOWN_WR,
  PSEUDO_TCP_SHUTDOWN_RDWR,
} PseudoTcpShutdown;

/**
 * PseudoTcpCallbacks:
 * @user_data: A user defined pointer to be passed to the callbacks
 * @PseudoTcpOpened: The #PseudoTcpSocket is now connected
 * @PseudoTcpReadable: The socket is readable
 * @PseudoTcpWritable: The socket is writable
 * @PseudoTcpClosed: The socket was closed (both sides)
 * @WritePacket: This callback is called when the socket needs to send data.
 *
 * A structure containing callbacks functions that will be called by the
 * #PseudoTcpSocket when some events happen.
 * <para> See also: #PseudoTcpWriteResult </para>
 *
 * Since: 0.0.11
 */
typedef struct {
  gpointer user_data;
  void  (*PseudoTcpOpened) (PseudoTcpSocket *tcp, gpointer data);
  void  (*PseudoTcpReadable) (PseudoTcpSocket *tcp, gpointer data);
  void  (*PseudoTcpWritable) (PseudoTcpSocket *tcp, gpointer data);
  void  (*PseudoTcpClosed) (PseudoTcpSocket *tcp, guint32 error, gpointer data);
  PseudoTcpWriteResult (*WritePacket) (PseudoTcpSocket *tcp,
      const gchar * buffer, guint32 len, gpointer data);
} PseudoTcpCallbacks;

/**
 * pseudo_tcp_socket_new:
 * @conversation: The conversation id for the socket.
 * @callbacks: A pointer to the #PseudoTcpCallbacks structure for getting
 * notified of the #PseudoTcpSocket events.
 *
 * Creates a new #PseudoTcpSocket for the specified conversation
 *
 <note>
   <para>
     The @callbacks must be non-NULL, in order to get notified of packets the
     socket needs to send.
   </para>
   <para>
     If the @callbacks structure was dynamicly allocated, it can be freed
     after the call @pseudo_tcp_socket_new
   </para>
 </note>
 *
 * Returns: The new #PseudoTcpSocket object, %NULL on error
 *
 * Since: 0.0.11
 */
PseudoTcpSocket *pseudo_tcp_socket_new (guint32 conversation,
    PseudoTcpCallbacks *callbacks);


/**
 * pseudo_tcp_socket_connect:
 * @self: The #PseudoTcpSocket object.
 *
 * Connects the #PseudoTcpSocket to the peer with the same conversation id.
 * The connection will only be successful after the
 * %PseudoTcpCallbacks:PseudoTcpOpened callback is called
 *
 * Returns: %TRUE on success, %FALSE on failure (not in %TCP_LISTEN state)
 * <para> See also: pseudo_tcp_socket_get_error() </para>
 *
 * Since: 0.0.11
 */
gboolean pseudo_tcp_socket_connect(PseudoTcpSocket *self);


/**
 * pseudo_tcp_socket_recv:
 * @self: The #PseudoTcpSocket object.
 * @buffer: The buffer to fill with received data
 * @len: The length of @buffer
 *
 * Receive data from the socket.
 *
 <note>
   <para>
     Only call this on the %PseudoTcpCallbacks:PseudoTcpReadable callback.
   </para>
   <para>
     This function should be called in a loop. If this function does not
     return -1 with EWOULDBLOCK as the error, the
     %PseudoTcpCallbacks:PseudoTcpReadable callback will not be called again.
   </para>
 </note>
 *
 * Returns: The number of bytes received or -1 in case of error
 * <para> See also: pseudo_tcp_socket_get_error() </para>
 *
 * Since: 0.0.11
 */
gint  pseudo_tcp_socket_recv(PseudoTcpSocket *self, char * buffer, size_t len);


/**
 * pseudo_tcp_socket_send:
 * @self: The #PseudoTcpSocket object.
 * @buffer: The buffer with data to send
 * @len: The length of @buffer
 *
 * Send data on the socket.
 *
 <note>
   <para>
     If this function return -1 with EWOULDBLOCK as the error, or if the return
     value is lower than @len, then the %PseudoTcpCallbacks:PseudoTcpWritable
     callback will be called when the socket will become writable.
   </para>
 </note>
 *
 * Returns: The number of bytes sent or -1 in case of error
 * <para> See also: pseudo_tcp_socket_get_error() </para>
 *
 * Since: 0.0.11
 */
gint pseudo_tcp_socket_send(PseudoTcpSocket *self, const char * buffer,
    guint32 len);


/**
 * pseudo_tcp_socket_close:
 * @self: The #PseudoTcpSocket object.
 * @force: %TRUE to close the socket forcefully, %FALSE to close it gracefully
 *
 * Close the socket for sending. If @force is set to %FALSE, the socket will
 * finish sending pending data before closing. If it is set to %TRUE, the socket
 * will discard pending data and close the connection immediately (sending a TCP
 * RST segment).
 *
 * The socket will be closed in both directions – sending and receiving – and
 * any pending received data must be read before calling this function, by
 * calling pseudo_tcp_socket_recv() until it blocks. If any pending data is in
 * the receive buffer when pseudo_tcp_socket_close() is called, a TCP RST
 * segment will be sent to the peer to notify it of the data loss.
 *
 <note>
   <para>
     The %PseudoTcpCallbacks:PseudoTcpClosed callback will not be called once
     the socket gets closed. It is only used for aborted connection.
     Instead, the socket gets closed when the pseudo_tcp_socket_get_next_clock()
     function returns FALSE.
   </para>
 </note>
 *
 * <para> See also: pseudo_tcp_socket_get_next_clock() </para>
 *
 * Since: 0.0.11
 */
void pseudo_tcp_socket_close(PseudoTcpSocket *self, gboolean force);

/**
 * pseudo_tcp_socket_shutdown:
 * @self: The #PseudoTcpSocket object.
 * @how: The directions of the connection to shut down.
 *
 * Shut down sending, receiving, or both on the socket, depending on the value
 * of @how. The behaviour of pseudo_tcp_socket_send() and
 * pseudo_tcp_socket_recv() will immediately change after this function returns
 * (depending on the value of @how), though the socket may continue to process
 * network traffic in the background even if sending or receiving data is
 * forbidden.
 *
 * This is equivalent to the POSIX shutdown() function. Setting @how to
 * %PSEUDO_TCP_SHUTDOWN_RDWR is equivalent to calling pseudo_tcp_socket_close().
 *
 * Since: 0.1.8
 */
void pseudo_tcp_socket_shutdown (PseudoTcpSocket *self, PseudoTcpShutdown how);

/**
 * pseudo_tcp_socket_get_error:
 * @self: The #PseudoTcpSocket object.
 *
 * Return the last encountered error.
 *
 <note>
   <para>
     The return value can be :
     <para>
       EINVAL (for pseudo_tcp_socket_connect()).
     </para>
     <para>
       EWOULDBLOCK or ENOTCONN (for pseudo_tcp_socket_recv() and
       pseudo_tcp_socket_send()).
     </para>
   </para>
 </note>
 *
 * Returns: The error code
 * <para> See also: pseudo_tcp_socket_connect() </para>
 * <para> See also: pseudo_tcp_socket_recv() </para>
 * <para> See also: pseudo_tcp_socket_send() </para>
 *
 * Since: 0.0.11
 */
int pseudo_tcp_socket_get_error(PseudoTcpSocket *self);


/**
 * pseudo_tcp_socket_get_next_clock:
 * @self: The #PseudoTcpSocket object.
 * @timeout: A pointer to be filled with the new timeout.
 *
 * Call this to determine the timeout needed before the next time call
 * to pseudo_tcp_socket_notify_clock() should be made.
 *
 * Returns: %TRUE if @timeout was filled, %FALSE if the socket is closed and
 * ready to be destroyed.
 *
 * <para> See also: pseudo_tcp_socket_notify_clock() </para>
 *
 * Since: 0.0.11
 */
gboolean pseudo_tcp_socket_get_next_clock(PseudoTcpSocket *self,
    guint64 *timeout);


/**
 * pseudo_tcp_socket_notify_clock:
 * @self: The #PseudoTcpSocket object.
 *
 * Start the processing of receiving data, pending data or syn/acks.
 * Call this based on timeout value returned by
 * pseudo_tcp_socket_get_next_clock().
 * It's ok to call this too frequently.
 *
 * <para> See also: pseudo_tcp_socket_get_next_clock() </para>
 *
 * Since: 0.0.11
 */
void pseudo_tcp_socket_notify_clock(PseudoTcpSocket *self);


/**
 * pseudo_tcp_socket_notify_mtu:
 * @self: The #PseudoTcpSocket object.
 * @mtu: The new MTU of the socket
 *
 * Set the MTU of the socket
 *
 * Since: 0.0.11
 */
void pseudo_tcp_socket_notify_mtu(PseudoTcpSocket *self, guint16 mtu);


/**
 * pseudo_tcp_socket_notify_packet:
 * @self: The #PseudoTcpSocket object.
 * @buffer: The buffer containing the received data
 * @len: The length of @buffer
 *
 * Notify the #PseudoTcpSocket when a new packet arrives
 *
 * Returns: %TRUE if the packet was processed successfully, %FALSE otherwise
 *
 * Since: 0.0.11
 */
gboolean pseudo_tcp_socket_notify_packet(PseudoTcpSocket *self,
    const gchar * buffer, guint32 len);


/**
 * pseudo_tcp_socket_notify_message:
 * @self: The #PseudoTcpSocket object.
 * @message: A #NiceInputMessage containing the received data.
 *
 * Notify the #PseudoTcpSocket that a new message has arrived, and enqueue the
 * data in its buffers to the #PseudoTcpSocket’s receive buffer.
 *
 * Returns: %TRUE if the packet was processed successfully, %FALSE otherwise
 *
 * Since: 0.1.5
 */
gboolean pseudo_tcp_socket_notify_message (PseudoTcpSocket *self,
    NiceInputMessage *message);


/**
 * pseudo_tcp_set_debug_level:
 * @level: The level of debug to set
 *
 * Sets the debug level to enable/disable normal/verbose debug messages.
 *
 * Since: 0.0.11
 */
void pseudo_tcp_set_debug_level (PseudoTcpDebugLevel level);


/**
 * pseudo_tcp_socket_get_available_bytes:
 * @self: The #PseudoTcpSocket object.
 *
 * Gets the number of bytes of data in the buffer that can be read without
 * receiving more packets from the network.
 *
 * Returns: The number of bytes or -1 if the connection is not established
 *
 * Since: 0.1.5
 */

gint pseudo_tcp_socket_get_available_bytes (PseudoTcpSocket *self);

/**
 * pseudo_tcp_socket_can_send:
 * @self: The #PseudoTcpSocket object.
 *
 * Returns if there is space in the send buffer to send any data.
 *
 * Returns: %TRUE if data can be sent, %FALSE otherwise
 *
 * Since: 0.1.5
 */

gboolean pseudo_tcp_socket_can_send (PseudoTcpSocket *self);

/**
 * pseudo_tcp_socket_get_available_send_space:
 * @self: The #PseudoTcpSocket object.
 *
 * Gets the number of bytes of space available in the transmission buffer.
 *
 * Returns: The number of bytes, or 0 if the connection is not established.
 *
 * Since: 0.1.5
 */
gsize pseudo_tcp_socket_get_available_send_space (PseudoTcpSocket *self);

/**
 * pseudo_tcp_socket_set_time:
 * @self: The #PseudoTcpSocket object.
 * @current_time: Current monotonic time, in milliseconds; or zero to use the
 * system monotonic clock.
 *
 * Sets the current monotonic time to be used by the TCP socket when calculating
 * timeouts and expiry times. If this function is not called, or is called with
 * @current_time as zero, g_get_monotonic_time() will be used. Otherwise, the
 * specified @current_time will be used until it is updated by calling this
 * function again.
 *
 * This function is intended for testing only, and should not be used in
 * production code.
 *
 * Since: 0.1.8
 */
void pseudo_tcp_socket_set_time (PseudoTcpSocket *self, guint32 current_time);

/**
 * pseudo_tcp_socket_is_closed:
 * @self: The #PseudoTcpSocket object.
 *
 * Gets whether the socket is closed, with the shutdown handshake completed,
 * and both peers no longer able to read or write data to the connection.
 *
 * Returns: %TRUE if the socket is closed in both directions, %FALSE otherwise
 * Since: 0.1.8
 */
gboolean pseudo_tcp_socket_is_closed (PseudoTcpSocket *self);

/**
 * pseudo_tcp_socket_is_closed_remotely:
 * @self: The #PseudoTcpSocket object.
 *
 * Gets whether the socket has been closed on the remote peer’s side of the
 * connection (i.e. whether pseudo_tcp_socket_close() has been called there).
 * This is guaranteed to return %TRUE if pseudo_tcp_socket_is_closed() returns
 * %TRUE. It will not return %TRUE after pseudo_tcp_socket_close() is called
 * until a FIN segment is received from the remote peer.
 *
 * Returns: %TRUE if the remote peer has closed its side of the connection,
 * %FALSE otherwise
 * Since: 0.1.8
 */
gboolean pseudo_tcp_socket_is_closed_remotely (PseudoTcpSocket *self);

G_END_DECLS

#endif /* __LIBNICE_PSEUDOTCP_H__ */

