# UOM registration and activation

QGroundControl can check real-name registration with China's UOM service, activate a supported aircraft, report its activation state, and deactivate it before cancelling its registration.
The aircraft owner completes real-name registration on the [official UOM website](https://uom.caac.gov.cn/).
QGroundControl does not collect identity documents or perform the owner's identity verification.

This feature supports the method 2 workflow in **GB 46761—2025, Registration and activation requirements for civil unmanned aircraft** (《民用无人驾驶航空器实名登记和激活要求》).
This mandatory national standard was published on October 31, 2025, and took effect on May 1, 2026; see the [official standard record](https://openstd.samr.gov.cn/bzgk/std/newGbInfo?hcno=495F3D69241E5961AFC4FA2939E383B4&refer=outter) and [CAAC standard text](https://www.caac.gov.cn/XXGK/XXGK/BZGF/BZGF_GJBZ/202601/P020260120370062157303.pdf).
Support for this workflow is not a statement of aircraft certification or complete conformity with the standard.

## Availability

The initial implementation supports Linux.
Open **Vehicle Configuration > UOM registration** to configure the connection.
When a device adapter is available, the separate **UOM** toolbar indicator displays registration, device activation, and platform acknowledgement status; select **Manage registration** to open the same page.
Device actions appear only when the vehicle's firmware plugin supplies a UOM device adapter.
The default Linux build includes the registration workflow but no device adapter; AMOV B950 operation requires the optional AMOV adapter and compatible aircraft firmware.

![UOM page with device actions hidden when no adapter is available](../../../assets/uom_registration/unavailable-adapter.png)

Screenshots on this page were captured from UI tests with synthetic aircraft identities and simulated workflow data.
They do not show real UOM receipts.

The adapter reads the aircraft's product code (SN).
The AMOV protocol does not provide product-model metadata, so the model field displays a generic device label.
It does not provision factory identity, edit RID parameters, or send flight telemetry to UOM.
Remote ID broadcasting remains a separate feature.

## Configure a connection

1. Obtain the producer ID, key ID, and three interface URLs for the intended environment from UOM through the aircraft producer's approved integration process.
2. Connect the aircraft, open **UOM registration**, and select **Read activation devices**.
   If more than one device is found, select the intended device explicitly.
3. Compare the displayed product code with the aircraft you intend to register or activate.
4. Select **Configure UOM connection**, choose **Integration** or **Official**, and enter the producer ID, expected aircraft SN, interface URLs, and key ID.
5. Save the configuration and check the credential-storage message.

![Connection configuration dialog with synthetic test values](../../../assets/uom_registration/connection.png)

The connection values in the screenshot are illustrative. Enter the URLs and credentials supplied for your environment.

The expected SN is an identity check; it does not change the device's product code.
Use the URLs supplied for the selected environment. Official URLs must not be replaced with integration URLs.
Credentials, registration results, and pending operations are separated by environment and connection profile.
This implementation accepts a 36-character producer ID, a 20-character printable ASCII product code without spaces, and a 32-digit hexadecimal SM4 key (hyphens in the supplied key ID are removed).
Interface URLs must use HTTPS on `uom.caac.gov.cn`, with no credentials, query, fragment, or nonstandard port.

The key ID is hidden while you enter it and is not written to ordinary settings or logs.
Leave the key field blank to keep the selected environment's saved key.
On Linux, QGroundControl uses the system Secret Service keyring when available.
If the build or desktop session cannot use the keyring, the page reports that credentials are kept only for the current session; enter the key again after restarting QGroundControl.

## Check registration and activate

The page displays three separate results:

| Result | Meaning |
| --- | --- |
| Real-name registration | The most recent registration check returned normal, cancelled, no record, or unknown. |
| Device activation | The state read from the connected device. |
| Latest UOM acknowledgement | The result of the most recent acknowledged activation-state report, not a query of the platform's current activation state. |

1. Select **Open official UOM website**, sign in or create an account, and complete the aircraft's real-name registration there.
2. Return to QGroundControl and select **Check real-name registration**.
3. Confirm that the aircraft identity matches and its registration is normal.
4. Select **Activate aircraft**, verify the target aircraft in the confirmation, and confirm.

QGroundControl requires device support and a confirmed device state before reporting activation to UOM.
The AMOV adapter requires both a persistence confirmation and a matching state from a subsequent device read; a transport acknowledgement alone is insufficient.
A successful platform report does not establish whether the aircraft's firmware enforces flight restrictions.

## Deactivate and cancel registration

Complete this process before transferring ownership or retiring the aircraft.

1. Select **Deactivate and cancel registration**.
2. Choose **Retired from use**, **Ownership transfer**, or **Other reason** and enter a reason.
   The reason is limited to 200 UTF-8 bytes; most Chinese characters use three bytes each.
3. Check the aircraft identity, acknowledge cancellation, and confirm.

![Deactivation confirmation showing an ownership transfer reason and an unchecked confirmation box](../../../assets/uom_registration/deactivation.png)

QGroundControl first confirms that the device is inactive, reports that state to UOM, and then submits the registration-cancellation request with its reason.
An interrupted operation remains pending until the remaining steps succeed.
If an aircraft fault prevents deactivation, follow the cancellation procedure on the official UOM website.

## Recover an interrupted operation

Reconnect the same aircraft and select **Retry pending operation** after a network failure, timeout, or restart.
The confirmation explains that a pending cancellation can resend the cancellation request.
Requests with identical parameters are separated by at least 60 seconds, including after an app restart.

While an operation is pending, its environment and aircraft identity cannot be changed.
You can correct its producer ID, URLs, or key in **Configure UOM connection** after acknowledging the pending-operation notice.
Saving corrections does not automatically write the device.
Continuing rechecks the device and registration and obtains a new cancellation token when required.

Audit records and recovery state are stored in `uom-registration.sqlite` in QGroundControl's app data directory.
Audit entries contain aircraft identifiers and operation details but exclude keys and cancellation tokens.
QGroundControl does not automatically delete these entries; preserve the app data needed for the standard's one-year audit-retention requirement.

## Aircraft firmware and validation

The flight controller and RID firmware must persist activation state and enforce flight restrictions before activation and after deactivation.
QGroundControl displays the available device checks but does not automatically change arming parameters.
Validate persistence across device restarts and firmware updates, flight restrictions, and UOM acknowledgements on the actual aircraft and firmware combination.
Software tests and successful interface exchanges alone do not establish whole-aircraft conformity or the standard's interface performance requirements.

## Build configuration

Follow the [source build instructions](../../qgc-dev-guide/getting_started/index.md) first.
`QGC_UOM_REGISTRATION` defaults to `ON` on Linux and `OFF` elsewhere; this initial implementation requires Linux when enabled.
It adds `OpenSSL::Crypto`, Qt Concurrent, and Qt SQL with the SQLite driver.
Linux packages need the corresponding runtime libraries. Install OpenSSL development headers to build; `libsecret` development headers enable optional keyring support.
Include the SQLite SQL driver when packaging the app.
Video support follows the normal source-build prerequisites; configure `QGC_ENABLE_GST_VIDEOSTREAMING=OFF` for a build without the streaming backend.
This option does not change the UOM workflow.

To build the optional AMOV adapter using the standard debug preset:

```bash
uv run --frozen --project tools --group build python tools/configure.py --preset default -- \
  -DQGC_UOM_REGISTRATION=ON -DQGC_UOM_AMOV_ADAPTER=ON
just build
```

`QGC_UOM_AMOV_ADAPTER` defaults to `OFF` and requires `QGC_UOM_REGISTRATION=ON`.
Only an AMOV-enabled build generates the private `MAVLink` extension used for its activation control and status messages.
The default `MAVLink` dialect is unchanged when the adapter is turned off.
To omit the feature, configure both options as `OFF`.

Other device integrations implement `UomDeviceAdapter` through `FirmwarePlugin::createUomDeviceAdapter(...)`.
Each vehicle owns its adapter; the common registration workflow does not depend on AMOV or general RID device-management features.
