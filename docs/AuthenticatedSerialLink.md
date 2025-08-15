# Authenticated Serial Link - QGroundControl

Tính năng này thêm khả năng kết nối COM port với xác thực người dùng thông qua tài khoản và mật khẩu.

## Tổng quan

Authenticated Serial Link cho phép QGroundControl kết nối đến các thiết bị serial sau khi thực hiện xác thực người dùng thông qua một server xác thực local. Điều này tăng cường bảo mật cho các kết nối serial quan trọng.

## Cách hoạt động

1. **Xác thực**: Trước khi kết nối serial port, ứng dụng sẽ kết nối đến authentication server
2. **Đăng nhập**: Gửi thông tin username/password đến server xác thực
3. **Nhận session**: Nhận session token nếu xác thực thành công
4. **Kết nối Serial**: Sau khi xác thực thành công, mới tiến hành kết nối serial port
5. **Truyền dữ liệu**: Dữ liệu chỉ được truyền khi đã xác thực thành công

## Cấu hình

Trong QGroundControl, khi tạo link mới, chọn loại "Authenticated Serial" và cấu hình:

### Authentication Settings
- **Username**: Tên đăng nhập
- **Password**: Mật khẩu
- **Auth Server**: Địa chỉ IP của authentication server (mặc định: 127.0.0.1)
- **Auth Port**: Port của authentication server (mặc định: 8080)

### Serial Port Settings  
- **Serial Port**: COM port cần kết nối
- **Baud Rate**: Tốc độ truyền (mặc định: 57600)
- **Advanced Settings**: Data bits, parity, stop bits, flow control

## Authentication Server

### Giao thức xác thực

Server xác thực sử dụng giao thức JSON qua TCP:

**Request format:**
```json
{
  "username": "user1",
  "password": "mypass",
  "action": "login"
}
```

**Success response:**
```json
{
  "status": "success",
  "message": "Welcome user1!",
  "session_token": "user1_1234567890_token",
  "user_info": {
    "username": "user1",
    "permissions": ["serial_access", "data_read", "data_write"]
  }
}
```

**Error response:**
```json
{
  "status": "error",
  "message": "Invalid username or password"
}
```

### Demo Server

Một demo authentication server được cung cấp trong `tools/auth_server_demo.py`:

```bash
# Chạy demo server
cd tools
python auth_server_demo.py
```

**Tài khoản demo:**
- Username: `admin`, Password: `password123`
- Username: `user1`, Password: `mypass`  
- Username: `drone_operator`, Password: `securepass`
- Username: `test`, Password: `test`

## Cài đặt và Build

1. **Thêm vào build**: Các file đã được thêm vào CMakeLists.txt và sẽ được build tự động

2. **Dependencies**: Cần Qt Network module (đã có sẵn trong QGroundControl)

## Sử dụng

1. **Khởi động authentication server**:
   ```bash
   python tools/auth_server_demo.py
   ```

2. **Trong QGroundControl**:
   - Vào Settings → Comm Links
   - Nhấn Add để tạo link mới
   - Chọn Type = "Authenticated Serial"
   - Nhập thông tin xác thực và cấu hình serial port
   - Connect

3. **Kiểm tra kết nối**:
   - Theo dõi console output của auth server
   - Kiểm tra trạng thái connection trong QGroundControl
   - Xem log messages để debug nếu cần

## Troubleshooting

### Lỗi thường gặp

1. **"Failed to connect to authentication server"**
   - Kiểm tra auth server có đang chạy không
   - Xác nhận địa chỉ IP và port đúng
   - Kiểm tra firewall/antivirus

2. **"Authentication failed"**
   - Kiểm tra username/password đúng
   - Xem log của auth server để biết chi tiết

3. **"Could not open port"**
   - Port đã được sử dụng bởi ứng dụng khác
   - Quyền truy cập serial port
   - Kiểm tra port name đúng

### Debug

Enable debug logging bằng cách set environment variable:
```bash
QT_LOGGING_RULES="qgc.comms.authenticatedserialllink.debug=true"
```

## Bảo mật

### Khuyến nghị
- Sử dụng mật khẩu mạnh
- Chỉ cho phép kết nối từ localhost (127.0.0.1)
- Implement HTTPS/TLS cho production server
- Thêm rate limiting để chống brute force
- Thêm session expiration và refresh

### Hạn chế hiện tại
- Mật khẩu truyền plaintext (cần encrypt trong production)
- Không có session management tự động
- Demo server không có persistent storage

## Phát triển thêm

### Tính năng có thể thêm
- **HTTPS/TLS**: Mã hóa connection đến auth server
- **Certificate authentication**: Sử dụng certificate thay vì password
- **LDAP/Active Directory**: Tích hợp với hệ thống xác thực doanh nghiệp
- **Multi-factor authentication**: Thêm OTP/2FA
- **Session management**: Auto refresh, timeout, logout
- **Audit logging**: Log tất cả authentication attempts
- **User roles**: Phân quyền chi tiết cho từng user

### Implementation notes
- Code follows QGroundControl pattern với Configuration/Worker/Link classes
- Thread-safe implementation với QThread và Qt signal/slot
- Compatible với existing LinkManager và UI framework
- Extensible design cho future enhancements

## Files được thêm/sửa đổi

### New files:
- `src/Comms/AuthenticatedSerialLink.h`
- `src/Comms/AuthenticatedSerialLink.cc`
- `src/UI/AppSettings/AuthenticatedSerialSettings.qml`
- `tools/auth_server_demo.py`
- `docs/AuthenticatedSerialLink.md` (this file)

### Modified files:
- `src/Comms/LinkConfiguration.h` - Added TypeAuthenticatedSerial enum
- `src/Comms/LinkConfiguration.cc` - Added factory methods
- `src/Comms/LinkManager.cc` - Added support for new link type
- `src/Comms/CMakeLists.txt` - Added new source files

## License

Tính năng này được phát hành dưới cùng license với QGroundControl (Apache 2.0).
