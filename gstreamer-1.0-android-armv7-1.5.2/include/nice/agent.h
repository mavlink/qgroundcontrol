/*
 * This file is part of the Nice GLib ICE library.
 *
 * (C) 2006-2010 Collabora Ltd.
 *  Contact: Youness Alaoui
 * (C) 2006-2010 Nokia Corporation. All rights reserved.
 *  Contact: Kai Vehmanen
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
 *   Dafydd Harries, Collabora Ltd.
 *   Youness Alaoui, Collabora Ltd.
 *   Kai Vehmanen, Nokia
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

#ifndef __LIBNICE_AGENT_H__
#define __LIBNICE_AGENT_H__

/**
 * SECTION:agent
 * @short_description:  ICE agent API implementation
 * @see_also: #NiceCandidate, #NiceAddress
 * @include: agent.h
 * @stability: Stable
 *
 * The #NiceAgent is your main object when using libnice.
 * It is the agent that will take care of everything relating to ICE.
 * It will take care of discovering your local candidates and do
 *  connectivity checks to create a stream of data between you and your peer.
 *
 * A #NiceAgent must always be used with a #GMainLoop running the #GMainContext
 * passed into nice_agent_new() (or nice_agent_new_reliable()). Without the
 * #GMainContext being iterated, the agent’s timers will not fire, etc.
 *
 * Streams and their components are referenced by integer IDs (with respect to a
 * given #NiceAgent). These IDs are guaranteed to be positive (i.e. non-zero)
 * for valid streams/components.
 *
 * To complete the ICE connectivity checks, the user must either register
 * an I/O callback (with nice_agent_attach_recv()) or call nice_agent_recv_messages()
 * in a loop on a dedicated thread.
 * Technically, #NiceAgent does not poll the streams on its own, since
 * user data could arrive at any time; to receive STUN packets
 * required for establishing ICE connectivity, it is backpiggying
 * on the facility chosen by the user. #NiceAgent will handle all STUN
 * packets internally; they're never actually passed to the I/O callback
 * or returned from nice_agent_recv_messages() and related functions.
 *
 * Each stream can receive data in one of two ways: using
 * nice_agent_attach_recv() or nice_agent_recv_messages() (and the derived
 * #NiceInputStream and #NiceIOStream classes accessible using
 * nice_agent_get_io_stream()). nice_agent_attach_recv() is non-blocking: it
 * takes a user-provided callback function and attaches the stream’s socket to
 * the provided #GMainContext, invoking the callback in that context for every
 * packet received. nice_agent_recv_messages() instead blocks on receiving a
 * packet, and writes it directly into a user-provided buffer. This reduces the
 * number of callback invokations and (potentially) buffer copies required to
 * receive packets. nice_agent_recv_messages() (or #NiceInputStream) is designed
 * to be used in a blocking loop in a separate thread.
 *
 * <example>
 *   <title>Simple example on how to use libnice</title>
 *   <programlisting>
 *   guint stream_id;
 *   gchar buffer[] = "hello world!";
 *   gchar *ufrag = NULL, *pwd = NULL;
 *   gchar *remote_ufrag, *remote_pwd;
 *   GSList *lcands = NULL;
 *
 *   // Create a nice agent, passing in the global default GMainContext.
 *   NiceAgent *agent = nice_agent_new (NULL, NICE_COMPATIBILITY_RFC5245);
 *   spawn_thread_to_run_main_loop (g_main_loop_new (NULL, FALSE));
 *
 *   // Connect the signals
 *   g_signal_connect (G_OBJECT (agent), "candidate-gathering-done",
 *                     G_CALLBACK (cb_candidate_gathering_done), NULL);
 *   g_signal_connect (G_OBJECT (agent), "component-state-changed",
 *                     G_CALLBACK (cb_component_state_changed), NULL);
 *   g_signal_connect (G_OBJECT (agent), "new-selected-pair",
 *                     G_CALLBACK (cb_new_selected_pair), NULL);
 *
 *   // Create a new stream with one component and start gathering candidates
 *   stream_id = nice_agent_add_stream (agent, 1);
 *   nice_agent_gather_candidates (agent, stream_id);
 *
 *   // Attach I/O callback the component to ensure that:
 *   // 1) agent gets its STUN packets (not delivered to cb_nice_recv)
 *   // 2) you get your own data
 *   nice_agent_attach_recv (agent, stream_id, 1, NULL,
 *                          cb_nice_recv, NULL);
 *
 *   // ... Wait until the signal candidate-gathering-done is fired ...
 *   lcands = nice_agent_get_local_candidates(agent, stream_id, 1);

 *   nice_agent_get_local_credentials(agent, stream_id, &ufrag, &pwd);
 *
 *   // ... Send local candidates and credentials to the peer
 *
 *   // Set the peer's remote credentials and remote candidates
 *   nice_agent_set_remote_credentials (agent, stream_id, remote_ufrag, remote_pwd);
 *   nice_agent_set_remote_candidates (agent, stream_id, 1, rcands);
 *
 *   // ... Wait until the signal new-selected-pair is fired ...
 *   // Send our message!
 *   nice_agent_send (agent, stream_id, 1, sizeof(buffer), buffer);
 *
 *   // Anything received will be received through the cb_nice_recv callback.
 *   // You must be running a GMainLoop on the global default GMainContext in
 *   // another thread for this to work.
 *
 *   // Destroy the object
 *   g_object_unref(agent);
 *
 *   </programlisting>
 * </example>
 *
 * Refer to the examples in the examples/ subdirectory of the libnice source for
 * more complete examples.
 *
 */


#include <glib-object.h>
#include <gio/gio.h>

/**
 * NiceAgent:
 *
 * The #NiceAgent is the main GObject of the libnice library and represents
 * the ICE agent.
 */
typedef struct _NiceAgent NiceAgent;

#include "address.h"
#include "candidate.h"
#include "debug.h"


G_BEGIN_DECLS

/**
 * NiceInputMessage:
 * @buffers: (array length=n_buffers): unowned array of #GInputVector buffers to
 * store data in for this message
 * @n_buffers: number of #GInputVectors in @buffers, or -1 to indicate @buffers
 * is %NULL-terminated
 * @from: (allow-none): return location to store the address of the peer who
 * transmitted the message, or %NULL
 * @length: total number of valid bytes contiguously stored in @buffers
 *
 * Represents a single message received off the network. For reliable
 * connections, this is essentially just an array of buffers (specifically,
 * @from can be ignored). for non-reliable connections, it represents a single
 * packet as received from the OS.
 *
 * @n_buffers may be -1 to indicate that @buffers is terminated by a
 * #GInputVector with a %NULL buffer pointer.
 *
 * By providing arrays of #NiceInputMessages to functions like
 * nice_agent_recv_messages(), multiple messages may be received with a single
 * call, which is more efficient than making multiple calls in a loop. In this
 * manner, nice_agent_recv_messages() is analogous to recvmmsg(); and
 * #NiceInputMessage to struct mmsghdr.
 *
 * Since: 0.1.5
 */
typedef struct {
  GInputVector *buffers;
  gint n_buffers;  /* may be -1 to indicate @buffers is NULL-terminated */
  NiceAddress *from;  /* return location for address of message sender */
  gsize length;  /* sum of the lengths of @buffers */
} NiceInputMessage;

/**
 * NiceOutputMessage:
 * @buffers: (array length=n_buffers): unowned array of #GOutputVector buffers
 * which contain data to transmit for this message
 * @n_buffers: number of #GOutputVectors in @buffers, or -1 to indicate @buffers
 * is %NULL-terminated
 *
 * Represents a single message to transmit on the network. For
 * reliable connections, this is essentially just an array of
 * buffer. for non-reliable connections, it represents a single packet
 * to send to the OS.
 *
 * @n_buffers may be -1 to indicate that @buffers is terminated by a
 * #GOutputVector with a %NULL buffer pointer.
 *
 * By providing arrays of #NiceOutputMessages to functions like
 * nice_agent_send_messages_nonblocking(), multiple messages may be transmitted
 * with a single call, which is more efficient than making multiple calls in a
 * loop. In this manner, nice_agent_send_messages_nonblocking() is analogous to
 * sendmmsg(); and #NiceOutputMessage to struct mmsghdr.
 *
 * Since: 0.1.5
 */
typedef struct {
  GOutputVector *buffers;
  gint n_buffers;
} NiceOutputMessage;


#define NICE_TYPE_AGENT nice_agent_get_type()

#define NICE_AGENT(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  NICE_TYPE_AGENT, NiceAgent))

#define NICE_AGENT_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  NICE_TYPE_AGENT, NiceAgentClass))

#define NICE_IS_AGENT(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  NICE_TYPE_AGENT))

#define NICE_IS_AGENT_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  NICE_TYPE_AGENT))

#define NICE_AGENT_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  NICE_TYPE_AGENT, NiceAgentClass))

typedef struct _NiceAgentClass NiceAgentClass;

struct _NiceAgentClass
{
  GObjectClass parent_class;
};


GType nice_agent_get_type (void);


/**
 * NICE_AGENT_MAX_REMOTE_CANDIDATES:
 *
 * A hard limit for the number of remote candidates. This
 * limit is enforced to protect against malevolent remote
 * clients.
 */
#define NICE_AGENT_MAX_REMOTE_CANDIDATES    25

/**
 * NiceComponentState:
 * @NICE_COMPONENT_STATE_DISCONNECTED: No activity scheduled
 * @NICE_COMPONENT_STATE_GATHERING: Gathering local candidates
 * @NICE_COMPONENT_STATE_CONNECTING: Establishing connectivity
 * @NICE_COMPONENT_STATE_CONNECTED: At least one working candidate pair
 * @NICE_COMPONENT_STATE_READY: ICE concluded, candidate pair selection
 * is now final
 * @NICE_COMPONENT_STATE_FAILED: Connectivity checks have been completed,
 * but connectivity was not established
 * @NICE_COMPONENT_STATE_LAST: Dummy state
 *
 * An enum representing the state of a component.
 * <para> See also: #NiceAgent::component-state-changed </para>
 */
typedef enum
{
  NICE_COMPONENT_STATE_DISCONNECTED,
  NICE_COMPONENT_STATE_GATHERING,
  NICE_COMPONENT_STATE_CONNECTING,
  NICE_COMPONENT_STATE_CONNECTED,
  NICE_COMPONENT_STATE_READY,
  NICE_COMPONENT_STATE_FAILED,
  NICE_COMPONENT_STATE_LAST
} NiceComponentState;


/**
 * NiceComponentType:
 * @NICE_COMPONENT_TYPE_RTP: RTP Component type
 * @NICE_COMPONENT_TYPE_RTCP: RTCP Component type
 *
 * Convenience enum representing the type of a component for use as the
 * component_id for RTP/RTCP usages.
 <example>
   <title>Example of use.</title>
   <programlisting>
   nice_agent_send (agent, stream_id, NICE_COMPONENT_TYPE_RTP, len, buf);
   </programlisting>
  </example>
 */
typedef enum
{
  NICE_COMPONENT_TYPE_RTP = 1,
  NICE_COMPONENT_TYPE_RTCP = 2
} NiceComponentType;


/**
 * NiceCompatibility:
 * @NICE_COMPATIBILITY_RFC5245: Use compatibility with the RFC5245 ICE-UDP specs
 * and RFC6544 ICE-TCP specs
 * @NICE_COMPATIBILITY_GOOGLE: Use compatibility for Google Talk specs
 * @NICE_COMPATIBILITY_MSN: Use compatibility for MSN Messenger specs
 * @NICE_COMPATIBILITY_WLM2009: Use compatibility with Windows Live Messenger
 * 2009
 * @NICE_COMPATIBILITY_OC2007: Use compatibility with Microsoft Office Communicator 2007
 * @NICE_COMPATIBILITY_OC2007R2: Use compatibility with Microsoft Office Communicator 2007 R2
 * @NICE_COMPATIBILITY_DRAFT19: Use compatibility for ICE Draft 19 specs
 * @NICE_COMPATIBILITY_LAST: Dummy last compatibility mode
 *
 * An enum to specify which compatible specifications the #NiceAgent should use.
 * Use with nice_agent_new()
 *
 * <warning>@NICE_COMPATIBILITY_DRAFT19 is deprecated and should not be used
 * in newly-written code. It is kept for compatibility reasons and
 * represents the same compatibility as @NICE_COMPATIBILITY_RFC5245 </warning>
 <note>
   <para>
   If @NICE_COMPATIBILITY_RFC5245 compatibility mode is used for a non-reliable
   agent, then ICE-UDP will be used with higher priority and ICE-TCP will also
   be used when the UDP connectivity fails. If it is used with a reliable agent,
   then ICE-UDP will be used with the TCP-Over-UDP (#PseudoTcpSocket) if ICE-TCP
   fails and ICE-UDP succeeds.
  </para>
 </note>
 *
 */
typedef enum
{
  NICE_COMPATIBILITY_RFC5245 = 0,
  NICE_COMPATIBILITY_DRAFT19 = NICE_COMPATIBILITY_RFC5245,
  NICE_COMPATIBILITY_GOOGLE,
  NICE_COMPATIBILITY_MSN,
  NICE_COMPATIBILITY_WLM2009,
  NICE_COMPATIBILITY_OC2007,
  NICE_COMPATIBILITY_OC2007R2,
  NICE_COMPATIBILITY_LAST = NICE_COMPATIBILITY_OC2007R2,
} NiceCompatibility;

/**
 * NiceProxyType:
 * @NICE_PROXY_TYPE_NONE: Do not use a proxy
 * @NICE_PROXY_TYPE_SOCKS5: Use a SOCKS5 proxy
 * @NICE_PROXY_TYPE_HTTP: Use an HTTP proxy
 * @NICE_PROXY_TYPE_LAST: Dummy last proxy type
 *
 * An enum to specify which proxy type to use for relaying.
 * Note that the proxies will only be used with TCP TURN relaying.
 * <para> See also: #NiceAgent:proxy-type </para>
 *
 * Since: 0.0.4
 */
typedef enum
{
  NICE_PROXY_TYPE_NONE = 0,
  NICE_PROXY_TYPE_SOCKS5,
  NICE_PROXY_TYPE_HTTP,
  NICE_PROXY_TYPE_LAST = NICE_PROXY_TYPE_HTTP,
} NiceProxyType;


/**
 * NiceAgentRecvFunc:
 * @agent: The #NiceAgent Object
 * @stream_id: The id of the stream
 * @component_id: The id of the component of the stream
 *        which received the data
 * @len: The length of the data
 * @buf: The buffer containing the data received
 * @user_data: The user data set in nice_agent_attach_recv()
 *
 * Callback function when data is received on a component
 *
 */
typedef void (*NiceAgentRecvFunc) (
  NiceAgent *agent, guint stream_id, guint component_id, guint len,
  gchar *buf, gpointer user_data);


/**
 * nice_agent_new:
 * @ctx: The Glib Mainloop Context to use for timers
 * @compat: The compatibility mode of the agent
 *
 * Create a new #NiceAgent.
 * The returned object must be freed with g_object_unref()
 *
 * Returns: The new agent GObject
 */
NiceAgent *
nice_agent_new (GMainContext *ctx, NiceCompatibility compat);


/**
 * nice_agent_new_reliable:
 * @ctx: The Glib Mainloop Context to use for timers
 * @compat: The compatibility mode of the agent
 *
 * Create a new #NiceAgent in reliable mode. If the connectivity is established
 * through ICE-UDP, then a #PseudoTcpSocket will be transparently used to
 * ensure reliability of the messages.
 * The returned object must be freed with g_object_unref()
 * <para> See also: #NiceAgent::reliable-transport-writable </para>
 *
 * Since: 0.0.11
 *
 * Returns: The new agent GObject
 */
NiceAgent *
nice_agent_new_reliable (GMainContext *ctx, NiceCompatibility compat);

/**
 * nice_agent_add_local_address:
 * @agent: The #NiceAgent Object
 * @addr: The address to listen to
 * If the port is 0, then a random port will be chosen by the system
 *
 * Add a local address from which to derive local host candidates for
 * candidate gathering.
 * <para>
 * Since 0.0.5, if this method is not called, libnice will automatically
 * discover the local addresses available
 * </para>
 *
 * See also: nice_agent_gather_candidates()
 * Returns: %TRUE on success, %FALSE on fatal (memory allocation) errors
 */
gboolean
nice_agent_add_local_address (NiceAgent *agent, NiceAddress *addr);


/**
 * nice_agent_add_stream:
 * @agent: The #NiceAgent Object
 * @n_components: The number of components to add to the stream
 *
 * Adds a data stream to @agent containing @n_components components. The
 * returned stream ID is guaranteed to be positive on success.
 *
 * Returns: The ID of the new stream, 0 on failure
 **/
guint
nice_agent_add_stream (
  NiceAgent *agent,
  guint n_components);

/**
 * nice_agent_remove_stream:
 * @agent: The #NiceAgent Object
 * @stream_id: The ID of the stream to remove
 *
 * Remove and free a previously created data stream from @agent. If any I/O
 * streams have been created using nice_agent_get_io_stream(), they should be
 * closed completely using g_io_stream_close() before this is called, or they
 * will get broken pipe errors.
 *
 **/
void
nice_agent_remove_stream (
  NiceAgent *agent,
  guint stream_id);


/**
 * nice_agent_set_port_range:
 * @agent: The #NiceAgent Object
 * @stream_id: The ID of the stream
 * @component_id: The ID of the component
 * @min_port: The minimum port to use
 * @max_port: The maximum port to use
 *
 * Sets a preferred port range for allocating host candidates.
 * <para>
 * If a local host candidate cannot be created on that port
 * range, then the nice_agent_gather_candidates() call will fail.
 * </para>
 * <para>
 * This MUST be called before nice_agent_gather_candidates()
 * </para>
 *
 */
void
nice_agent_set_port_range (
    NiceAgent *agent,
    guint stream_id,
    guint component_id,
    guint min_port,
    guint max_port);

/**
 * nice_agent_set_relay_info:
 * @agent: The #NiceAgent Object
 * @stream_id: The ID of the stream
 * @component_id: The ID of the component
 * @server_ip: The IP address of the TURN server
 * @server_port: The port of the TURN server
 * @username: The TURN username to use for the allocate
 * @password: The TURN password to use for the allocate
 * @type: The type of relay to use
 *
 * Sets the settings for using a relay server during the candidate discovery.
 * This may be called multiple times to add multiple relay servers to the
 * discovery process; one TCP and one UDP, for example.
 *
 * Returns: %TRUE if the TURN settings were accepted.
 * %FALSE if the address was invalid.
 */
gboolean nice_agent_set_relay_info(
    NiceAgent *agent,
    guint stream_id,
    guint component_id,
    const gchar *server_ip,
    guint server_port,
    const gchar *username,
    const gchar *password,
    NiceRelayType type);

/**
 * nice_agent_gather_candidates:
 * @agent: The #NiceAgent object
 * @stream_id: The ID of the stream to start
 *
 * Allocate and start listening on local candidate ports and start the remote
 * candidate gathering process.
 * Once done, #NiceAgent::candidate-gathering-done is called for the stream.
 * As soon as this function is called, #NiceAgent::new-candidate signals may be
 * emitted, even before this function returns.
 *
 * nice_agent_get_local_candidates() will only return non-empty results after
 * calling this function.
 *
 * <para>See also: nice_agent_add_local_address()</para>
 * <para>See also: nice_agent_set_port_range()</para>
 *
 * Returns: %FALSE if the stream ID is invalid or if a host candidate couldn't
 * be allocated on the requested interfaces/ports; %TRUE otherwise
 *
 <note>
   <para>
    Local addresses can be previously set with nice_agent_add_local_address()
  </para>
  <para>
    Since 0.0.5, If no local address was previously added, then the nice agent
    will automatically detect the local address using
    nice_interfaces_get_local_ips()
   </para>
 </note>
 */
gboolean
nice_agent_gather_candidates (
  NiceAgent *agent,
  guint stream_id);

/**
 * nice_agent_set_remote_credentials:
 * @agent: The #NiceAgent Object
 * @stream_id: The ID of the stream
 * @ufrag: nul-terminated string containing an ICE username fragment
 *    (length must be between 22 and 256 chars)
 * @pwd: nul-terminated string containing an ICE password
 *    (length must be between 4 and 256 chars)
 *
 * Sets the remote credentials for stream @stream_id.
 *
 <note>
   <para>
     Stream credentials do not override per-candidate credentials if set
   </para>
   <para>
     Due to the native of peer-reflexive candidates, any agent using a per-stream
     credentials (RFC5245, WLM2009, OC2007R2 and DRAFT19) instead of
     per-candidate credentials (GOOGLE, MSN, OC2007), must
     use the nice_agent_set_remote_credentials() API instead of setting the
     username and password on the candidates.
   </para>
 </note>
 *
 * Returns: %TRUE on success, %FALSE on error.
 */
gboolean
nice_agent_set_remote_credentials (
  NiceAgent *agent,
  guint stream_id,
  const gchar *ufrag, const gchar *pwd);


/**
 * nice_agent_set_local_credentials:
 * @agent: The #NiceAgent Object
 * @stream_id: The ID of the stream
 * @ufrag: nul-terminated string containing an ICE username fragment
 *    (length must be between 22 and 256 chars)
 * @pwd: nul-terminated string containing an ICE password
 *    (length must be between 4 and 256 chars)
 *
 * Sets the local credentials for stream @stream_id.
 *
 <note>
   <para>
     This is only effective before ICE negotiation has started.
   </para>
 </note>
 *
 * Since 0.1.11
 * Returns: %TRUE on success, %FALSE on error.
 */
gboolean
nice_agent_set_local_credentials (
  NiceAgent *agent,
  guint stream_id,
  const gchar *ufrag,
  const gchar *pwd);


/**
 * nice_agent_get_local_credentials:
 * @agent: The #NiceAgent Object
 * @stream_id: The ID of the stream
 * @ufrag: (out callee-allocates): return location for a nul-terminated string
 * containing an ICE username fragment; must be freed with g_free()
 * @pwd: (out callee-allocates): return location for a nul-terminated string
 * containing an ICE password; must be freed with g_free()
 *
 * Gets the local credentials for stream @stream_id. This may be called any time
 * after creating a stream using nice_agent_add_stream().
 *
 * An error will be returned if this is called for a non-existent stream, or if
 * either of @ufrag or @pwd are %NULL.
 *
 * Returns: %TRUE on success, %FALSE on error.
 */
gboolean
nice_agent_get_local_credentials (
  NiceAgent *agent,
  guint stream_id,
  gchar **ufrag, gchar **pwd);

/**
 * nice_agent_set_remote_candidates:
 * @agent: The #NiceAgent Object
 * @stream_id: The ID of the stream the candidates are for
 * @component_id: The ID of the component the candidates are for
 * @candidates: (element-type NiceCandidate) (transfer none): a #GSList of
 * #NiceCandidate items describing each candidate to add
 *
 * Sets, adds or updates the remote candidates for a component of a stream.
 *
 <note>
   <para>
    NICE_AGENT_MAX_REMOTE_CANDIDATES is the absolute maximum limit
    for remote candidates.
   </para>
   <para>
   You must first call nice_agent_gather_candidates() and wait for the
   #NiceAgent::candidate-gathering-done signale before
   calling nice_agent_set_remote_candidates()
   </para>
   <para>
    Since 0.1.3, there is no need to wait for the candidate-gathering-done signal.
    Remote candidates can be set even while gathering local candidates.
    Newly discovered local candidates will automatically be paired with
    existing remote candidates.
   </para>
 </note>
 *
 * Returns: The number of candidates added, negative on errors (memory
 * allocation error or invalid component)
 **/
int
nice_agent_set_remote_candidates (
  NiceAgent *agent,
  guint stream_id,
  guint component_id,
  const GSList *candidates);


/**
 * nice_agent_send:
 * @agent: The #NiceAgent Object
 * @stream_id: The ID of the stream to send to
 * @component_id: The ID of the component to send to
 * @len: The length of the buffer to send
 * @buf: The buffer of data to send
 *
 * Sends a data payload over a stream's component.
 *
 <note>
   <para>
     Component state MUST be NICE_COMPONENT_STATE_READY, or as a special case,
     in any state if component was in READY state before and was then restarted
   </para>
   <para>
   In reliable mode, the -1 error value means either that you are not yet
   connected or that the send buffer is full (equivalent to EWOULDBLOCK).
   In both cases, you simply need to wait for the
   #NiceAgent::reliable-transport-writable signal to be fired before resending
   the data.
   </para>
   <para>
   In non-reliable mode, it will virtually never happen with UDP sockets, but
   it might happen if the active candidate is a TURN-TCP connection that got
   disconnected.
   </para>
   <para>
   In both reliable and non-reliable mode, a -1 error code could also mean that
   the stream_id and/or component_id are invalid.
   </para>
</note>
 *
 * Returns: The number of bytes sent, or negative error code
 */
gint
nice_agent_send (
  NiceAgent *agent,
  guint stream_id,
  guint component_id,
  guint len,
  const gchar *buf);

/**
 * nice_agent_send_messages_nonblocking:
 * @agent: a #NiceAgent
 * @stream_id: the ID of the stream to send to
 * @component_id: the ID of the component to send to
 * @messages: (array length=n_messages): array of messages to send, of at least
 * @n_messages entries in length
 * @n_messages: number of entries in @messages
 * @cancellable: (allow-none): a #GCancellable to cancel the operation from
 * another thread, or %NULL
 * @error: (allow-none): return location for a #GError, or %NULL
 *
 * Sends multiple messages on the socket identified by the given
 * stream/component pair. Transmission is non-blocking, so a
 * %G_IO_ERROR_WOULD_BLOCK error may be returned if the send buffer is full.
 *
 * As with nice_agent_send(), the given component must be in
 * %NICE_COMPONENT_STATE_READY or, as a special case, in any state if it was
 * previously ready and was then restarted.
 *
 * On success, the number of messages written to the socket will be returned,
 * which may be less than @n_messages if transmission would have blocked
 * part-way through. Zero will be returned if @n_messages is zero, or if
 * transmission would have blocked on the first message.
 *
 * In reliable mode, it is instead recommended to use
 * nice_agent_send().  The return value can be less than @n_messages
 * or 0 even if it is still possible to send a partial message. In
 * this case, "nice-agent-writable" will never be triggered, so the
 * application would have to use nice_agent_sent() to fill the buffer or have
 * to retry sending at a later point.
 *
 * On failure, -1 will be returned and @error will be set. If the #NiceAgent is
 * reliable and the socket is not yet connected, %G_IO_ERROR_BROKEN_PIPE will be
 * returned; if the write buffer is full, %G_IO_ERROR_WOULD_BLOCK will be
 * returned. In both cases, wait for the #NiceAgent::reliable-transport-writable
 * signal before trying again. If the given @stream_id or @component_id are
 * invalid or not yet connected, %G_IO_ERROR_BROKEN_PIPE will be returned.
 * %G_IO_ERROR_FAILED will be returned for other errors.
 *
 * Returns: the number of messages sent (may be zero), or -1 on error
 *
 * Since: 0.1.5
 */
gint
nice_agent_send_messages_nonblocking (
    NiceAgent *agent,
    guint stream_id,
    guint component_id,
    const NiceOutputMessage *messages,
    guint n_messages,
    GCancellable *cancellable,
    GError **error);

/**
 * nice_agent_get_local_candidates:
 * @agent: The #NiceAgent Object
 * @stream_id: The ID of the stream
 * @component_id: The ID of the component
 *
 * Retrieve from the agent the list of all local candidates
 * for a stream's component
 *
 <note>
   <para>
     The caller owns the returned GSList as well as the candidates contained
     within it.
     To get full results, the client should wait for the
     #NiceAgent::candidate-gathering-done signal.
   </para>
 </note>
 *
 * Returns: (element-type NiceCandidate) (transfer full): a #GSList of
 * #NiceCandidate objects representing the local candidates of @agent
 **/
GSList *
nice_agent_get_local_candidates (
  NiceAgent *agent,
  guint stream_id,
  guint component_id);


/**
 * nice_agent_get_remote_candidates:
 * @agent: The #NiceAgent Object
 * @stream_id: The ID of the stream
 * @component_id: The ID of the component
 *
 * Get a list of the remote candidates set on a stream's component
 *
 <note>
   <para>
     The caller owns the returned GSList as well as the candidates contained
     within it.
   </para>
   <para>
     The list of remote candidates can change during processing.
     The client should register for the #NiceAgent::new-remote-candidate signal
     to get notified of new remote candidates.
   </para>
 </note>
 *
 * Returns: (element-type NiceCandidate) (transfer full): a #GSList of
 * #NiceCandidates objects representing the remote candidates set on the @agent
 **/
GSList *
nice_agent_get_remote_candidates (
  NiceAgent *agent,
  guint stream_id,
  guint component_id);

/**
 * nice_agent_restart:
 * @agent: The #NiceAgent Object
 *
 * Restarts the session as defined in ICE draft 19. This function
 * needs to be called both when initiating (ICE spec section 9.1.1.1.
 * "ICE Restarts"), as well as when reacting (spec section 9.2.1.1.
 * "Detecting ICE Restart") to a restart.
 *
 * Returns: %TRUE on success %FALSE on error
 **/
gboolean
nice_agent_restart (
  NiceAgent *agent);

/**
 * nice_agent_restart_stream:
 * @agent: The #NiceAgent Object
 * @stream_id: The ID of the stream
 *
 * Restarts a single stream as defined in RFC 5245. This function
 * needs to be called both when initiating (ICE spec section 9.1.1.1.
 * "ICE Restarts"), as well as when reacting (spec section 9.2.1.1.
 * "Detecting ICE Restart") to a restart.
 *
 * Unlike nice_agent_restart(), this applies to a single stream. It also
 * does not generate a new tie breaker.
 *
 * Returns: %TRUE on success %FALSE on error
 *
 * Since: 0.1.6
 **/
gboolean
nice_agent_restart_stream (
    NiceAgent *agent,
    guint stream_id);


/**
 * nice_agent_attach_recv: (skip)
 * @agent: The #NiceAgent Object
 * @stream_id: The ID of stream
 * @component_id: The ID of the component
 * @ctx: The Glib Mainloop Context to use for listening on the component
 * @func: The callback function to be called when data is received on
 * the stream's component (will not be called for STUN messages that
 * should be handled by #NiceAgent itself)
 * @data: user data associated with the callback
 *
 * Attaches the stream's component's sockets to the Glib Mainloop Context in
 * order to be notified whenever data becomes available for a component,
 * and to enable #NiceAgent to receive STUN messages (during the
 * establishment of ICE connectivity).
 *
 * This must not be used in combination with nice_agent_recv_messages() (or
 * #NiceIOStream or #NiceInputStream) on the same stream/component pair.
 *
 * Calling nice_agent_attach_recv() with a %NULL @func will detach any existing
 * callback and cause reception to be paused for the given stream/component
 * pair. You must iterate the previously specified #GMainContext sufficiently to
 * ensure all pending I/O callbacks have been received before calling this
 * function to unset @func, otherwise data loss of received packets may occur.
 *
 * Returns: %TRUE on success, %FALSE if the stream or component IDs are invalid.
 */
gboolean
nice_agent_attach_recv (
  NiceAgent *agent,
  guint stream_id,
  guint component_id,
  GMainContext *ctx,
  NiceAgentRecvFunc func,
  gpointer data);

/**
 * nice_agent_recv:
 * @agent: a #NiceAgent
 * @stream_id: the ID of the stream to receive on
 * @component_id: the ID of the component to receive on
 * @buf: (array length=buf_len) (out caller-allocates): caller-allocated buffer
 * to write the received data into, of length at least @buf_len
 * @buf_len: length of @buf
 * @cancellable: (allow-none): a #GCancellable to allow the operation to be
 * cancelled from another thread, or %NULL
 * @error: (allow-none): return location for a #GError, or %NULL
 *
 * A single-message version of nice_agent_recv_messages().
 *
 * Returns: the number of bytes written to @buf on success (guaranteed to be
 * greater than 0 unless @buf_len is 0), 0 if in reliable mode and the remote
 * peer closed the stream, or -1 on error
 *
 * Since: 0.1.5
 */
gssize
nice_agent_recv (
    NiceAgent *agent,
    guint stream_id,
    guint component_id,
    guint8 *buf,
    gsize buf_len,
    GCancellable *cancellable,
    GError **error);

/**
 * nice_agent_recv_messages:
 * @agent: a #NiceAgent
 * @stream_id: the ID of the stream to receive on
 * @component_id: the ID of the component to receive on
 * @messages: (array length=n_messages) (out caller-allocates): caller-allocated
 * array of #NiceInputMessages to write the received messages into, of length at
 * least @n_messages
 * @n_messages: number of entries in @messages
 * @cancellable: (allow-none): a #GCancellable to allow the operation to be
 * cancelled from another thread, or %NULL
 * @error: (allow-none): return location for a #GError, or %NULL
 *
 * Block on receiving data from the given stream/component combination on
 * @agent, returning only once exactly @n_messages messages have been received
 * and written into @messages, the stream is closed by the other end or by
 * calling nice_agent_remove_stream(), or @cancellable is cancelled.
 *
 * Any STUN packets received will not be added to @messages; instead,
 * they'll be passed for processing to #NiceAgent itself. Since #NiceAgent
 * does not poll for messages on its own, it's therefore essential to keep
 * calling this function for ICE connection establishment to work.
 *
 * In the non-error case, in reliable mode, this will block until all buffers in
 * all @n_messages have been filled with received data (i.e. @messages is
 * treated as a large, flat array of buffers). In non-reliable mode, it will
 * block until @n_messages messages have been received, each of which does not
 * have to fill all the buffers in its #NiceInputMessage. In the non-reliable
 * case, each #NiceInputMessage must have enough buffers to contain an entire
 * message (65536 bytes), or any excess data may be silently dropped.
 *
 * For each received message, #NiceInputMessage::length will be set to the
 * number of valid bytes stored in the message’s buffers. The bytes are stored
 * sequentially in the buffers; there are no gaps apart from at the end of the
 * buffer array (in non-reliable mode). If non-%NULL on input,
 * #NiceInputMessage::from will have the address of the sending peer stored in
 * it. The base addresses, sizes, and number of buffers in each message will not
 * be modified in any case.
 *
 * This must not be used in combination with nice_agent_attach_recv() on the
 * same stream/component pair.
 *
 * If the stream/component pair doesn’t exist, or if a suitable candidate socket
 * hasn’t yet been selected for it, a %G_IO_ERROR_BROKEN_PIPE error will be
 * returned. A %G_IO_ERROR_CANCELLED error will be returned if the operation was
 * cancelled. %G_IO_ERROR_FAILED will be returned for other errors.
 *
 * Returns: the number of valid messages written to @messages on success
 * (guaranteed to be greater than 0 unless @n_messages is 0), 0 if the remote
 * peer closed the stream, or -1 on error
 *
 * Since: 0.1.5
 */
gint
nice_agent_recv_messages (
    NiceAgent *agent,
    guint stream_id,
    guint component_id,
    NiceInputMessage *messages,
    guint n_messages,
    GCancellable *cancellable,
    GError **error);

/**
 * nice_agent_recv_nonblocking:
 * @agent: a #NiceAgent
 * @stream_id: the ID of the stream to receive on
 * @component_id: the ID of the component to receive on
 * @buf: (array length=buf_len) (out caller-allocates): caller-allocated buffer
 * to write the received data into, of length at least @buf_len
 * @buf_len: length of @buf
 * @cancellable: (allow-none): a #GCancellable to allow the operation to be
 * cancelled from another thread, or %NULL
 * @error: (allow-none): return location for a #GError, or %NULL
 *
 * A single-message version of nice_agent_recv_messages_nonblocking().
 *
 * Returns: the number of bytes received into @buf on success (guaranteed to be
 * greater than 0 unless @buf_len is 0), 0 if in reliable mode and the remote
 * peer closed the stream, or -1 on error
 *
 * Since: 0.1.5
 */
gssize
nice_agent_recv_nonblocking (
    NiceAgent *agent,
    guint stream_id,
    guint component_id,
    guint8 *buf,
    gsize buf_len,
    GCancellable *cancellable,
    GError **error);

/**
 * nice_agent_recv_messages_nonblocking:
 * @agent: a #NiceAgent
 * @stream_id: the ID of the stream to receive on
 * @component_id: the ID of the component to receive on
 * @messages: (array length=n_messages) (out caller-allocates): caller-allocated
 * array of #NiceInputMessages to write the received messages into, of length at
 * least @n_messages
 * @n_messages: number of entries in @messages
 * @cancellable: (allow-none): a #GCancellable to allow the operation to be
 * cancelled from another thread, or %NULL
 * @error: (allow-none): return location for a #GError, or %NULL
 *
 * Try to receive data from the given stream/component combination on @agent,
 * without blocking. If receiving data would block, -1 is returned and
 * %G_IO_ERROR_WOULD_BLOCK is set in @error. If any other error occurs, -1 is
 * returned and @error is set accordingly. Otherwise, 0 is returned if (and only
 * if) @n_messages is 0. In all other cases, the number of valid messages stored
 * in @messages is returned, and will be greater than 0.
 *
 * This function behaves similarly to nice_agent_recv_messages(), except that it
 * will not block on filling (in reliable mode) or receiving (in non-reliable
 * mode) exactly @n_messages messages. In reliable mode, it will receive bytes
 * into @messages until it would block; in non-reliable mode, it will receive
 * messages until it would block.
 *
 * Any STUN packets received will not be added to @messages; instead,
 * they'll be passed for processing to #NiceAgent itself. Since #NiceAgent
 * does not poll for messages on its own, it's therefore essential to keep
 * calling this function for ICE connection establishment to work.
 *
 * As this function is non-blocking, @cancellable is included only for parity
 * with nice_agent_recv_messages(). If @cancellable is cancelled before this
 * function is called, a %G_IO_ERROR_CANCELLED error will be returned
 * immediately.
 *
 * This must not be used in combination with nice_agent_attach_recv() on the
 * same stream/component pair.
 *
 * Returns: the number of valid messages written to @messages on success
 * (guaranteed to be greater than 0 unless @n_messages is 0), 0 if in reliable
 * mode and the remote peer closed the stream, or -1 on error
 *
 * Since: 0.1.5
 */
gint
nice_agent_recv_messages_nonblocking (
    NiceAgent *agent,
    guint stream_id,
    guint component_id,
    NiceInputMessage *messages,
    guint n_messages,
    GCancellable *cancellable,
    GError **error);

/**
 * nice_agent_set_selected_pair:
 * @agent: The #NiceAgent Object
 * @stream_id: The ID of the stream
 * @component_id: The ID of the component
 * @lfoundation: The local foundation of the candidate to use
 * @rfoundation: The remote foundation of the candidate to use
 *
 * Sets the selected candidate pair for media transmission
 * for a given stream's component. Calling this function will
 * disable all further ICE processing (connection check,
 * state machine updates, etc). Note that keepalives will
 * continue to be sent.
 *
 * Returns: %TRUE on success, %FALSE if the candidate pair cannot be found
 */
gboolean
nice_agent_set_selected_pair (
  NiceAgent *agent,
  guint stream_id,
  guint component_id,
  const gchar *lfoundation,
  const gchar *rfoundation);

/**
 * nice_agent_get_selected_pair:
 * @agent: The #NiceAgent Object
 * @stream_id: The ID of the stream
 * @component_id: The ID of the component
 * @local: The local selected candidate
 * @remote: The remote selected candidate
 *
 * Retreive the selected candidate pair for media transmission
 * for a given stream's component.
 *
 * Returns: %TRUE on success, %FALSE if there is no selected candidate pair
 */
gboolean
nice_agent_get_selected_pair (
  NiceAgent *agent,
  guint stream_id,
  guint component_id,
  NiceCandidate **local,
  NiceCandidate **remote);

/**
 * nice_agent_get_selected_socket:
 * @agent: The #NiceAgent Object
 * @stream_id: The ID of the stream
 * @component_id: The ID of the component
 *
 * Retreive the local socket associated with the selected candidate pair
 * for media transmission for a given stream's component.
 *
 * This is useful for adding ICE support to legacy applications that already
 * have a protocol that maintains a connection. If the socket is duplicated
 * before unrefing the agent, the application can take over and continue to use
 * it. New applications are encouraged to use the built in libnice stream
 * handling instead and let libnice handle the connection maintenance.
 *
 * Users of this method are encouraged to not use a TURN relay or any kind
 * of proxy, as in this case, the socket will not be available to the
 * application because the packets are encapsulated.
 *
 * Returns: (transfer full) (nullable): pointer to the #GSocket, or %NULL if
 * there is no selected candidate or if the selected candidate is a relayed
 * candidate.
 *
 * Since: 0.1.5
 */
GSocket *
nice_agent_get_selected_socket (
  NiceAgent *agent,
  guint stream_id,
  guint component_id);

/**
 * nice_agent_set_selected_remote_candidate:
 * @agent: The #NiceAgent Object
 * @stream_id: The ID of the stream
 * @component_id: The ID of the component
 * @candidate: The #NiceCandidate to select
 *
 * Sets the selected remote candidate for media transmission
 * for a given stream's component. This is used to force the selection of
 * a specific remote candidate even when connectivity checks are failing
 * (e.g. non-ICE compatible candidates).
 * Calling this function will disable all further ICE processing
 * (connection check, state machine updates, etc). Note that keepalives will
 * continue to be sent.
 *
 * Returns: %TRUE on success, %FALSE on failure
 */
gboolean
nice_agent_set_selected_remote_candidate (
  NiceAgent *agent,
  guint stream_id,
  guint component_id,
  NiceCandidate *candidate);


/**
 * nice_agent_set_stream_tos:
 * @agent: The #NiceAgent Object
 * @stream_id: The ID of the stream
 * @tos: The ToS to set
 *
 * Sets the IP_TOS and/or IPV6_TCLASS field on the stream's sockets' options
 *
 * Since: 0.0.9
 */
void nice_agent_set_stream_tos (
  NiceAgent *agent,
  guint stream_id,
  gint tos);



/**
 * nice_agent_set_software:
 * @agent: The #NiceAgent Object
 * @software: The value of the SOFTWARE attribute to add.
 *
 * This function will set the value of the SOFTWARE attribute to be added to
 * STUN requests, responses and error responses sent during connectivity checks.
 * <para>
 * The SOFTWARE attribute will only be added in the #NICE_COMPATIBILITY_RFC5245
 * and #NICE_COMPATIBILITY_WLM2009 compatibility modes.
 *
 * </para>
 * <note>
     <para>
       The @software argument will be appended with the libnice version before
       being sent.
     </para>
     <para>
       The @software argument must be in UTF-8 encoding and only the first
       128 characters will be sent.
     </para>
   </note>
 *
 * Since: 0.0.10
 *
 */
void nice_agent_set_software (
    NiceAgent *agent,
    const gchar *software);

/**
 * nice_agent_set_stream_name:
 * @agent: The #NiceAgent Object
 * @stream_id: The ID of the stream to change
 * @name: The new name of the stream or %NULL
 *
 * This function will assign a media type to a stream. The only values
 * that can be used to produce a valid SDP are: "audio", "video",
 * "text", "application", "image" and "message".
 *
 * This is only useful when parsing and generating an SDP of the
 * candidates.
 *
 * <para>See also: nice_agent_generate_local_sdp()</para>
 * <para>See also: nice_agent_parse_remote_sdp()</para>
 * <para>See also: nice_agent_get_stream_name()</para>
 *
 * Returns: %TRUE if the name has been set. %FALSE in case of error
 * (invalid stream or duplicate name).
 * Since: 0.1.4
 */
gboolean nice_agent_set_stream_name (
    NiceAgent *agent,
    guint stream_id,
    const gchar *name);

/**
 * nice_agent_get_stream_name:
 * @agent: The #NiceAgent Object
 * @stream_id: The ID of the stream to change
 *
 * This function will return the name assigned to a stream.

 * <para>See also: nice_agent_set_stream_name()</para>
 *
 * Returns: The name of the stream. The name is only valid while the stream
 * exists or until it changes through a call to nice_agent_set_stream_name().
 *
 *
 * Since: 0.1.4
 */
const gchar *nice_agent_get_stream_name (
    NiceAgent *agent,
    guint stream_id);

/**
 * nice_agent_get_default_local_candidate:
 * @agent: The #NiceAgent Object
 * @stream_id: The ID of the stream
 * @component_id: The ID of the component
 *
 * This helper function will return the recommended default candidate to be
 * used for non-ICE compatible clients. This will usually be the candidate
 * with the lowest priority, since it will be the longest path but the one with
 * the most chances of success.
 * <note>
     <para>
     This function is only useful in order to manually generate the
     local SDP
     </para>
 * </note>
 *
 * Returns: The candidate to be used as the default candidate, or %NULL in case
 * of error. Must be freed with nice_candidate_free() once done.
 *
 */
NiceCandidate *
nice_agent_get_default_local_candidate (
    NiceAgent *agent,
    guint stream_id,
    guint component_id);

/**
 * nice_agent_generate_local_sdp:
 * @agent: The #NiceAgent Object
 *
 * Generate an SDP string containing the local candidates and credentials for
 * all streams and components in the agent.
 *
 <note>
   <para>
     The SDP will not contain any codec lines and the 'm' line will not list
     any payload types.
   </para>
   <para>
    It is highly recommended to set names on the streams prior to calling this
    function. Unnamed streams will show up as '-' in the 'm' line, but the SDP
    will not be parseable with nice_agent_parse_remote_sdp() if a stream is
    unnamed.
   </para>
   <para>
     The default candidate in the SDP will be selected based on the lowest
     priority candidate for the first component.
   </para>
 </note>
 *
 * <para>See also: nice_agent_set_stream_name() </para>
 * <para>See also: nice_agent_parse_remote_sdp() </para>
 * <para>See also: nice_agent_generate_local_stream_sdp() </para>
 * <para>See also: nice_agent_generate_local_candidate_sdp() </para>
 * <para>See also: nice_agent_get_default_local_candidate() </para>
 *
 * Returns: A string representing the local SDP. Must be freed with g_free()
 * once done.
 *
 * Since: 0.1.4
 **/
gchar *
nice_agent_generate_local_sdp (
  NiceAgent *agent);

/**
 * nice_agent_generate_local_stream_sdp:
 * @agent: The #NiceAgent Object
 * @stream_id: The ID of the stream
 * @include_non_ice: Whether or not to include non ICE specific lines
 * (m=, c= and a=rtcp: lines)
 *
 * Generate an SDP string containing the local candidates and credentials
 * for a stream.
 *
 <note>
   <para>
     The SDP will not contain any codec lines and the 'm' line will not list
     any payload types.
   </para>
   <para>
    It is highly recommended to set the name of the stream prior to calling this
    function. Unnamed streams will show up as '-' in the 'm' line.
   </para>
   <para>
     The default candidate in the SDP will be selected based on the lowest
     priority candidate.
   </para>
 </note>
 *
 * <para>See also: nice_agent_set_stream_name() </para>
 * <para>See also: nice_agent_parse_remote_stream_sdp() </para>
 * <para>See also: nice_agent_generate_local_sdp() </para>
 * <para>See also: nice_agent_generate_local_candidate_sdp() </para>
 * <para>See also: nice_agent_get_default_local_candidate() </para>
 *
 * Returns: A string representing the local SDP for the stream. Must be freed
 * with g_free() once done.
 *
 * Since: 0.1.4
 **/
gchar *
nice_agent_generate_local_stream_sdp (
    NiceAgent *agent,
    guint stream_id,
    gboolean include_non_ice);

/**
 * nice_agent_generate_local_candidate_sdp:
 * @agent: The #NiceAgent Object
 * @candidate: The candidate to generate
 *
 * Generate an SDP string representing a local candidate.
 *
 * <para>See also: nice_agent_parse_remote_candidate_sdp() </para>
 * <para>See also: nice_agent_generate_local_sdp() </para>
 * <para>See also: nice_agent_generate_local_stream_sdp() </para>
 *
 * Returns: A string representing the SDP for the candidate. Must be freed
 * with g_free() once done.
 *
 * Since: 0.1.4
 **/
gchar *
nice_agent_generate_local_candidate_sdp (
    NiceAgent *agent,
    NiceCandidate *candidate);

/**
 * nice_agent_parse_remote_sdp:
 * @agent: The #NiceAgent Object
 * @sdp: The remote SDP to parse
 *
 * Parse an SDP string and extracts candidates and credentials from it and sets
 * them on the agent.
 *
 <note>
   <para>
    This function will return an error if a stream has not been assigned a name
    with nice_agent_set_stream_name() as it becomes troublesome to assign the
    streams from the agent to the streams in the SDP.
   </para>
 </note>
 *
 *
 * <para>See also: nice_agent_set_stream_name() </para>
 * <para>See also: nice_agent_generate_local_sdp() </para>
 * <para>See also: nice_agent_parse_remote_stream_sdp() </para>
 * <para>See also: nice_agent_parse_remote_candidate_sdp() </para>
 *
 * Returns: The number of candidates added, negative on errors
 *
 * Since: 0.1.4
 **/
int
nice_agent_parse_remote_sdp (
    NiceAgent *agent,
    const gchar *sdp);


/**
 * nice_agent_parse_remote_stream_sdp:
 * @agent: The #NiceAgent Object
 * @stream_id: The ID of the stream to parse
 * @sdp: The remote SDP to parse
 * @ufrag: Pointer to store the ice ufrag if non %NULL. Must be freed with
 * g_free() after use
 * @pwd: Pointer to store the ice password if non %NULL. Must be freed with
 * g_free() after use
 *
 * Parse an SDP string representing a single stream and extracts candidates
 * and credentials from it.
 *
 * <para>See also: nice_agent_generate_local_stream_sdp() </para>
 * <para>See also: nice_agent_parse_remote_sdp() </para>
 * <para>See also: nice_agent_parse_remote_candidate_sdp() </para>
 *
 * Returns: (element-type NiceCandidate) (transfer full): A #GSList of
 * candidates parsed from the SDP, or %NULL in case of errors
 *
 * Since: 0.1.4
 **/
GSList *
nice_agent_parse_remote_stream_sdp (
    NiceAgent *agent,
    guint stream_id,
    const gchar *sdp,
    gchar **ufrag,
    gchar **pwd);


/**
 * nice_agent_parse_remote_candidate_sdp:
 * @agent: The #NiceAgent Object
 * @stream_id: The ID of the stream the candidate belongs to
 * @sdp: The remote SDP to parse
 *
 * Parse an SDP string and extracts the candidate from it.
 *
 * <para>See also: nice_agent_generate_local_candidate_sdp() </para>
 * <para>See also: nice_agent_parse_remote_sdp() </para>
 * <para>See also: nice_agent_parse_remote_stream_sdp() </para>
 *
 * Returns: The parsed candidate or %NULL if there was an error.
 *
 * Since: 0.1.4
 **/
NiceCandidate *
nice_agent_parse_remote_candidate_sdp (
    NiceAgent *agent,
    guint stream_id,
    const gchar *sdp);

/**
 * nice_agent_get_io_stream:
 * @agent: A #NiceAgent
 * @stream_id: The ID of the stream to wrap
 * @component_id: The ID of the component to wrap
 *
 * Gets a #GIOStream wrapper around the given stream and component in
 * @agent. The I/O stream will be valid for as long as @stream_id is valid.
 * The #GInputStream and #GOutputStream implement #GPollableInputStream and
 * #GPollableOutputStream.
 *
 * This function may only be called on reliable #NiceAgents. It is a
 * programming error to try and create an I/O stream wrapper for an
 * unreliable stream.
 *
 * Returns: (transfer full): A #GIOStream.
 *
 * Since: 0.1.5
 */
GIOStream *
nice_agent_get_io_stream (
    NiceAgent *agent,
    guint stream_id,
    guint component_id);

/**
 * nice_component_state_to_string:
 * @state: a #NiceComponentState
 *
 * Returns a string representation of the state, generally to use in debug
 * messages.
 *
 * Returns: (transfer none): a string representation of @state
 * Since: 0.1.6
 */
const gchar *
nice_component_state_to_string (NiceComponentState state);

/**
 * nice_agent_forget_relays:
 * @agent: The #NiceAgent Object
 * @stream_id: The ID of the stream
 * @component_id: The ID of the component
 *
 * Forget all the relay servers previously added using
 * nice_agent_set_relay_info(). Currently connected streams will keep
 * using the relay as long as they have not been restarted and haven't
 * succesfully negotiated a different path.
 *
 * Returns: %FALSE if the component could not be found, %TRUE otherwise
 *
 * Since: 0.1.6
 */
gboolean
nice_agent_forget_relays (NiceAgent *agent,
    guint stream_id,
    guint component_id);

/**
 * nice_agent_get_component_state:
 * @agent: The #NiceAgent Object
 * @stream_id: The ID of the stream
 * @component_id: The ID of the component
 *
 * Retrieves the current state of a component.
 *
 * Returns: the #NiceComponentState of the component and
 * %NICE_COMPONENT_STATE_FAILED if the component was invalid.
 *
 * Since: 0.1.8
 */
NiceComponentState
nice_agent_get_component_state (NiceAgent *agent,
    guint stream_id,
    guint component_id);

G_END_DECLS

#endif /* __LIBNICE_AGENT_H__ */
