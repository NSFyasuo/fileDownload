# 文件共享/上传下载服务器

一个简单的C语言HTTP服务器，支持文件上传、下载、共享和用户管理。

## 功能

- 用户注册和登录
- 文件上传（需登录）
- 文件下载
- 文件共享（生成分享链接）
- 通过分享链接下载
- 文件列表查看

## 构建

```bash
make
```

## 运行

```bash
./file_server
```

服务器在8080端口运行。

## 使用

访问 http://localhost:8080 进行注册、登录、上传和下载文件。

## 依赖

- GCC
- SQLite3

## 结构

- `src/` - 源代码
- `include/` - 头文件
- `web/` - Web界面
- `uploads/` - 上传文件目录

## 部署

1. 编译项目：`make`
2. 运行服务器：`./file_server`
3. 可添加systemd服务：

创建 `/etc/systemd/system/file_server.service`：

```
[Unit]
Description=File Share Server
After=network.target

[Service]
Type=simple
User=your_user
WorkingDirectory=/path/to/fileDownload
ExecStart=/path/to/fileDownload/file_server
Restart=always

[Install]
WantedBy=multi-user.target
```

然后：

```bash
sudo systemctl enable file_server
sudo systemctl start file_server
```