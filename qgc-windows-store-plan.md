# QGroundControl — Windows Store Publishing Plan

## Overview

This document covers everything required to publish QGroundControl to the Microsoft Store, replacing the existing NSIS installer with MSIX packaging. One MSIX package serves both direct download and Store distribution. The rollout strategy is to publish daily builds to a Store flight group first, then launch stable to the main listing once the pipeline is proven and stable 5.1 is ready.

---

## Purchases & Accounts

### Microsoft Partner Center Account
- **Cost:** $19 one-time registration fee (individual) or free for registered companies
- **URL:** https://partner.microsoft.com
- Required to register the app, manage flights, and submit packages
- Sign up with the org's Microsoft account (not a personal account)

### Code Signing Certificate
- **Type:** OV (Organisation Validation) minimum; EV (Extended Validation) recommended
- **Recommended CA:** DigiCert or Sectigo — both support direct issuance into Azure Key Vault, meaning the private key is generated inside the HSM and never exported
- EV is recommended because it bypasses Windows SmartScreen reputation warnings immediately for direct downloads; OV works but SmartScreen will warn until enough download reputation is built
- One cert signs all builds — daily, stable, forever — until it expires, then renew and re-issue
- **Note:** As of February 2026, code signing certs are capped at 1-year validity by the CA/Browser Forum, so all certs are now annual

#### Pricing (approximate, 2026)

| CA | OV | EV |
|---|---|---|
| Sectigo | ~$220/year | ~$280/year |
| DigiCert | ~$400/year | ~$560/year |

Sectigo is significantly cheaper for equivalent trust. DigiCert's premium is brand recognition and enterprise support, not technical superiority. For an open source project, Sectigo OV or EV is the practical choice.

The difference between OV and EV for QGC specifically: EV (~$60 more/year with Sectigo) is worth it because QGC is distributed as a direct download as well as via the Store. Without EV, every new release binary triggers SmartScreen warnings for direct download users until enough download volume is accumulated — which can take weeks for each new version.

### Azure Subscription
- Required for Azure Key Vault (HSM cert storage) and Azure AD (CI authentication)
- **Sign-up:** Standard pay-as-you-go account at portal.azure.com — just a Microsoft account and a credit card, no enterprise agreement or commitment required. The same account and tenant covers both Key Vault and the Azure AD app registration for Partner Center API credentials.
- **Key Vault tier:** Premium is required (Standard does not support HSM-backed keys, which the CA mandates for OV/EV certs)
- **Key Vault cost:** ~$12/year for the HSM-protected key storage ($1/month per key), plus $0.03 per 10,000 signing operations — at any realistic build frequency the per-operation charge is negligible (1,000 builds/day would cost ~$1/month)
- If the project already has Azure resources this may already exist

---

## One-Time Manual Setup

These steps cannot be automated and must be done before any CI work begins.

### 1. Register the App in Partner Center
- Reserve the app name "QGroundControl"
- Note the following values — they go into the MSIX manifest:
  - **Publisher ID** (e.g. `CN=XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX`)
  - **Package Identity Name** (e.g. `MAVLinkQGroundControl`)
  - **Package Family Name**

### 2. Create a Package Flight
- In Partner Center, create a flight group called "Daily"
- Add developers and testers by Microsoft account email
- Note the **Flight ID** — needed for the CI submission step
- The main listing can remain unpublished (draft) while the flight is active; flight group members install via a direct link rather than searching the Store

### 3. Issue the Code Signing Cert into Azure Key Vault
- Create an Azure Key Vault instance
- Purchase OV/EV cert from DigiCert or Sectigo with direct Key Vault issuance
- Note the **Key Vault URL** and **Certificate Name**

### 4. Create an Azure AD App for CI
- Register an Azure AD application in the same tenant
- Grant it Partner Center Submission API permissions and Key Vault signing permissions
- Note **Tenant ID**, **Client ID**, **Client Secret**

### 5. Microsoft runFullTrust Approval
- On the first Store submission, Microsoft's review team must approve the `runFullTrust` restricted capability
- This is routinely granted for existing Win32 desktop apps
- Allow extra review time for the first submission only

---

## Installer Strategy: NSIS → MSIX

MSIX replaces NSIS entirely as the Windows installer format. It is not an additional artifact alongside NSIS — NSIS is removed.

MSIX works as a standalone installer for direct download (users double-click a `.msix` file to install, just like an `.exe`) as well as for Store distribution. One package format serves both channels. The staged install tree that NSIS currently wraps is handed to `MakeAppx.exe` instead — the build output itself is unchanged.

Benefits over NSIS:
- Guaranteed clean uninstall with no registry orphans
- No admin required to install (installs per-user by default)
- Automatic rollback if install fails
- Delta updates via `.appinstaller` files for self-hosted distribution
- Single package format for both direct download and the Store

The only prerequisite for direct download installs is that the MSIX is signed with a cert from a trusted CA (covered in the cert section above). A self-signed cert requires the user to enable developer mode, so a proper CA-issued cert is essential.

---

## Code & Packaging Changes

### Remove
- `cmake/install/CPack/CreateCPackNSIS.cmake`
- `deploy/windows/nullsoft_installer.nsi`
- `deploy/windows/installheader.bmp`
- NSIS-related logic in `cmake/install/`

### Add

#### `deploy/windows/Package.appxmanifest`
Defines the app's Store identity and capabilities. Key fields populated from Partner Center registration:

```xml
<Package
  xmlns="http://schemas.microsoft.com/appx/manifest/foundation/windows10"
  xmlns:rescap="http://schemas.microsoft.com/appx/manifest/foundation/windows10/restrictedcapabilities"
  xmlns:uap="http://schemas.microsoft.com/appx/manifest/uap/windows10">

  <Identity
    Name="MAVLinkQGroundControl"
    Publisher="CN=XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX"
    Version="x.x.x.0" />

  <Properties>
    <DisplayName>QGroundControl</DisplayName>
    <PublisherDisplayName>MAVLink</PublisherDisplayName>
    <Logo>Assets\StoreLogo.png</Logo>
  </Properties>

  <Applications>
    <Application Id="QGroundControl" Executable="bin\QGroundControl.exe" EntryPoint="Windows.FullTrustApplication">
      <uap:VisualElements DisplayName="QGroundControl" ... />
      <!-- Execution aliases for GPU modes -->
      <Extensions>
        <uap:Extension Category="windows.appExecutionAlias">
          <uap:AppExecutionAlias>
            <uap:ExecutionAlias Alias="QGroundControl.exe" />
          </uap:AppExecutionAlias>
        </uap:Extension>
      </Extensions>
    </Application>
  </Applications>

  <Capabilities>
    <rescap:Capability Name="runFullTrust" />
  </Capabilities>

</Package>
```

#### `deploy/windows/Assets/`
Store-required visual assets at mandated sizes:

| File | Size |
|---|---|
| `StoreLogo.png` | 50×50 |
| `Square44x44Logo.png` | 44×44 |
| `Square150x150Logo.png` | 150×150 |
| `Wide310x150Logo.png` | 310×150 |
| `Square310x310Logo.png` | 310×310 |
| `SplashScreen.png` | 620×300 |

#### `cmake/install/CPack/CreateCPackMSIX.cmake` (or a post-install script)
Replaces `CreateCPackNSIS.cmake`. After the install tree is staged, runs:

```cmake
# Configure manifest with version number
configure_file(
    "${CMAKE_SOURCE_DIR}/deploy/windows/Package.appxmanifest.in"
    "${CMAKE_BINARY_DIR}/Package.appxmanifest"
    @ONLY
)

# Copy manifest and assets into staged install tree
file(COPY "${CMAKE_BINARY_DIR}/Package.appxmanifest" DESTINATION "${CMAKE_INSTALL_PREFIX}")
file(COPY "${CMAKE_SOURCE_DIR}/deploy/windows/Assets" DESTINATION "${CMAKE_INSTALL_PREFIX}")

# Pack MSIX (MakeAppx.exe must be on PATH via Windows SDK)
execute_process(
    COMMAND MakeAppx.exe pack /d "${CMAKE_INSTALL_PREFIX}" /p "${CMAKE_BINARY_DIR}/${QGC_MSIX_NAME}"
)
```

Signing is handled in CI (see below) rather than at CMake time.

---

## CI Changes (`windows.yml`)

### Remove
- "Create Installer" step (runs NSIS/CPack)
- "Install and verify installer" step (NSIS-specific)

### Add

#### Sign the MSIX
Uses AzureSignTool — no hardware token required, signs via Key Vault API:

```yaml
- name: Install AzureSignTool
  run: dotnet tool install --global AzureSignTool

- name: Sign MSIX
  run: |
    AzureSignTool sign `
      --azure-key-vault-url ${{ secrets.AKV_URL }} `
      --azure-key-vault-client-id ${{ secrets.AZURE_CLIENT_ID }} `
      --azure-key-vault-client-secret ${{ secrets.AZURE_CLIENT_SECRET }} `
      --azure-key-vault-tenant-id ${{ secrets.AZURE_TENANT_ID }} `
      --azure-key-vault-certificate ${{ secrets.AKV_CERT_NAME }} `
      --timestamp-rfc3161 http://timestamp.digicert.com `
      QGroundControl.msix
  shell: pwsh
```

#### Verify MSIX (optional smoke test)
```yaml
- name: Verify MSIX
  shell: pwsh
  run: |
    Add-AppxPackage -Path QGroundControl.msix
    $app = Get-AppxPackage -Name "MAVLinkQGroundControl"
    if (-not $app) { Write-Error "MSIX install failed"; exit 1 }
```

#### Publish to Store Flight (every master merge)
```yaml
- name: Publish to Daily Flight
  if: github.ref == 'refs/heads/master'
  uses: microsoft/store-publish@v1
  with:
    tenant-id: ${{ secrets.AZURE_TENANT_ID }}
    client-id: ${{ secrets.AZURE_CLIENT_ID }}
    client-secret: ${{ secrets.AZURE_CLIENT_SECRET }}
    app-id: ${{ secrets.STORE_APP_ID }}
    flight-id: ${{ secrets.STORE_DAILY_FLIGHT_ID }}
    package-path: QGroundControl.msix
```

#### Publish to Store Main Listing (tags/releases only)
```yaml
- name: Publish to Store (Stable)
  if: startsWith(github.ref, 'refs/tags/v')
  uses: microsoft/store-publish@v1
  with:
    tenant-id: ${{ secrets.AZURE_TENANT_ID }}
    client-id: ${{ secrets.AZURE_CLIENT_ID }}
    client-secret: ${{ secrets.AZURE_CLIENT_SECRET }}
    app-id: ${{ secrets.STORE_APP_ID }}
    package-path: QGroundControl.msix
```

### New GitHub Secrets Required

| Secret | Description |
|---|---|
| `AKV_URL` | Azure Key Vault URL |
| `AKV_CERT_NAME` | Certificate name within Key Vault |
| `AZURE_TENANT_ID` | Azure AD tenant ID (shared with Store submission) |
| `AZURE_CLIENT_ID` | Azure AD app client ID (shared with Store submission) |
| `AZURE_CLIENT_SECRET` | Azure AD app client secret (shared with Store submission) |
| `STORE_APP_ID` | App ID from Partner Center |
| `STORE_DAILY_FLIGHT_ID` | Flight ID for the Daily flight group |

Note: `AZURE_TENANT_ID`, `AZURE_CLIENT_ID`, and `AZURE_CLIENT_SECRET` are shared between signing and Store submission — one Azure AD app, same credentials for both.

---

## What Doesn't Change

- The build pipeline (CMake, Qt, GStreamer)
- The binary (`QGroundControl.exe`) and all DLLs
- AWS upload of the artifact for direct download
- The staged install tree that was previously handed to NSIS is handed to `MakeAppx.exe` instead

---

## Rollout Sequence

1. Complete purchases (Partner Center account, code signing cert, Azure subscription)
2. Register app in Partner Center, create Daily flight group, issue cert into Key Vault, create Azure AD app
3. Implement MSIX packaging and CI changes
4. Begin publishing daily builds to the flight group — this validates the full pipeline and triggers `runFullTrust` review
5. Once pipeline is stable, cert approved, and stable 5.1 is ready, publish first stable to the main Store listing
