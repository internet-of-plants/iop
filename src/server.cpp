#include "iop/server.hpp"

#include "iop-hal/wifi.hpp"
#include "iop-hal/thread.hpp"

#include "iop-hal/network.hpp"
#include "iop-hal/device.hpp"
#include "iop/api.hpp"
#include "iop/loop.hpp"

namespace iop {
auto pageHTMLStart() -> iop::StaticString {
  return IOP_STR(
    "<!DOCTYPE HTML>\r\n"
    "<html><body><div class='center'>\r\n"
    "  <h1>Internet of Plants</h1>\r\n"
    "  <form class='center' action='/submit' method='POST'>\r\n");
}

auto wifiOverwriteHTML() -> iop::StaticString {
  return IOP_STR(
    "    <div class='border'>\r\n"
    "      <h3>WiFi Credentials</h3>\r\n"
    "      <em>Already connected to the network, if you want to update the WiFi credentials please check the box below and fill the fields.</em>\r\n"
    "      <div class='overwrite'>\r\n"
    "        <input type='checkbox' name='wifi'>\r\n"
    "        <label for='wifi'>Overwrite wifi credentials</label>\r\n"
    "      </div>\r\n"
    "      <div class='center'>\r\n"
    "        <div class='wifi input-padding' style='display: none'>\r\n"
    "          <span>Network name:</span>\r\n"
    "          <input name='ssid' type='text' class='input' />\r\n"
    "        </div>\r\n"
    "        <div class='wifi' style='display: none'>\r\n"
    "          <span>Password:</span>\r\n"
    "          <input name='password' type='password' class='input' />\r\n"
    "        </div>\r\n"
    "      </div>\r\n"
    "    </div>\r\n");
}

auto wifiHTML() -> iop::StaticString {
  return IOP_STR(
    "    <div class='border'>\r\n"
    "      <h3>WiFi Credentials</h3>\r\n"
    "      <div><input type='hidden' value='true' name='wifi'></div>\r\n"
    "      <div class='center'>\r\n"
    "        <div class='wifi input-padding'>\r\n"
    "          <span>Network name:</span>\r\n"
    "          <input name='ssid' type='text' class='input' />\r\n"
    "        </div>\r\n"
    "        <div class='wifi'>\r\n"
    "          <span>Password:</span>\r\n"
    "          <input name='password' type='password' class='input' />\r\n"
    "        </div>\r\n"
    "      </div>\r\n"
    "    </div>\r\n");
}

auto iopOverwriteHTML() -> iop::StaticString {
  return IOP_STR(
    "    <div class='border'>\r\n"
    "      <h3>Internet of Plants Credentials</h3>\r\n"
    "      <em>Already authenticated with the server, if you want to update the Internet of Plants credentials please check the box below and fill the fields.</em\r\n"
    "      <div class='overwrite'>\r\n"
    "        <input type='checkbox' name='iop'>\r\n"
    "        <label for='iop'>Overwrite Internet of Plants Credentials</label>\r\n"
    "      </div>\r\n"
    "      <div class='center'>\r\n"
    "        <div class='iop input-padding' style='display: none'>\r\n"
    "          <span>Email:</span>\r\n"
    "          <input name='iopEmail' type='text' class='input'' />\r\n"
    "        </div>\r\n"
    "        <div class='iop' style='display: none'>\r\n"
    "          <span>Password:</span>\r\n"
    "          <input name='iopPassword' type='password' class='input' />\r\n"
    "        </div>\r\n"
    "      </div>\r\n"
    "    </div>\r\n");
}

auto iopHTML() -> iop::StaticString {
  return IOP_STR(
    "    <div class='border'>\r\n"
    "      <h3>Internet of Plants Credentials</h3>\r\n"
    "      <input type='hidden' value='true' name='iop' />\r\n"
    "      <div>\r\n"
    "        <div class='iop input-padding'>\r\n"
    "          <span>Email:</span>\r\n"
    "          <input name='iopEmail' type='text' class='input' />\r\n"
    "        </div>\r\n"
    "        <div class='iop'>\r\n"
    "          <span>Password:</span>\r\n"
    "          <input name='iopPassword' type='password' class='input' />\r\n"
    "        </div>\r\n"
    "      </div>\r\n"
    "    </div>\r\n");
}

auto script() -> iop::StaticString {
  return IOP_STR(
    "    <script type='application/javascript'>\r\n"
    "    function toggleDisplay(className, checked) {\r\n"
    "      for (const el of document.getElementsByClassName(className)) {\r\n"
    "        if (checked === true) {\r\n"
    "          el.style.display = 'block';\r\n"
    "        } else if (checked === false) {\r\n"
    "          el.style.display = 'none';\r\n"
    "        }\r\n"
    "      }\r\n"
    "    }\r\n"
    "    document.querySelector(\"input[name='wifi']\").addEventListener('change', ev => toggleDisplay('wifi', ev.currentTarget.checked));\r\n"
    "    document.querySelector(\"input[name='iop']\").addEventListener('change', ev => toggleDisplay('iop', ev.currentTarget.checked));\r\n"
    "    for (const el of document.getElementsByClassName('wifi')) {\r\n"
    "      toggleDisplay('wifi', el.checked);\r\n"
    "    }\r\n"
    "    for (const el of document.getElementsByClassName('iop')) {\r\n"
    "      toggleDisplay('iop', el.checked);\r\n"
    "    }\r\n"
    "    </script>\r\n");
}
auto css() -> iop::StaticString {
  return IOP_STR(
    "    <style>\r\n"
    "    html, body {\r\n"
    "      height: 100%;\r\n"
    "      width: 100%;\r\n"
    "      display: flex;\r\n"
    "      align-items: center;\r\n"
    "      font-family: Tahoma, sans-serif;\r\n"
    "      background-color: #DFDFDF;\r\n"
    "      margin: 0;\r\n"
    "    }\r\n"
    "    .center {\r\n"
    "      display: flex;\r\n"
    "      width: 100%;\r\n"
    "      flex-direction: column;\r\n"
    "      align-items: center;\r\n"
    "      text-align: center;\r\n"
    "    }\r\n"
    "    h3 { margin-top: 0; }\r\n"
    "    h1 { padding-top: 2em; }\r\n"
    "    .border {\r\n"
    "      border: 1px solid;\r\n"
    "      border-radius: 0.125rem;\r\n"
    "      padding: 1em;\r\n"
    "      margin-top: 3em;\r\n"
    "    }\r\n"
    "    .wifi, .iop {\r\n"
    "      width: 300px;\r\n"
    "    }\r\n"
    "    form { width: 500px !important; }\r\n"
    "    .submit {\r\n"
    "      background-color: #A3A3A3;\r\n"
    "      border: solid 1px #626262;\r\n"
    "      border-radius: 3px;\r\n"
    "      margin-top: 1em;\r\n"
    "      color: #070707;\r\n"
    "    }\r\n"
    "    input { background-color: #DFDFDF; }\r\n"
    "    .input {\r\n"
    "      float: right;\r\n"
    "      border: solid 1px #626262;\r\n"
    "      border-radius: 3px;\r\n"
    "      padding-left: 5px;\r\n"
    "      height: calc(100% - 2px);\r\n"
    "    }\r\n"
    "    .overwrite { margin: 1em 0; }\r\n"
    "    .input-padding { padding-bottom: 1em; }\r\n"
    "    </style>\r\n");
}

auto pageHTMLEnd() -> iop::StaticString {
  return IOP_STR(
    "    <input type='submit' value='Submit' class='submit' />\r\n"
    "  </form>\r\n"
    "</div></body></html>");
}

auto CredentialsServer::setup() noexcept -> void {
  IOP_TRACE();
  this->server.on(IOP_STR("/favicon.ico"), [](iop_hal::HttpConnection &conn, iop::Log &logger) { conn.send(404, IOP_STR("text/plain"), IOP_STR("")); (void) logger; });
  this->server.on(IOP_STR("/submit"), [this](iop_hal::HttpConnection &conn, iop::Log &logger) {
    IOP_TRACE();
    logger.debugln(IOP_STR("Received credentials form"));

    const auto wifi = conn.arg(IOP_STR("wifi"));
    const auto ssid = conn.arg(IOP_STR("ssid"));
    const auto psk = conn.arg(IOP_STR("password"));
    if (wifi && ssid && psk) {
      logger.debug(IOP_STR("SSID: "));
      logger.debugln(*ssid);
      this->credentialsWifi = std::unique_ptr<DynamicCredential>(new (std::nothrow) DynamicCredential(*ssid, *psk));
      iop_assert(this->credentialsWifi, IOP_STR("Unable to allocate credentialsWifi"));
    }

    const auto iop = conn.arg(IOP_STR("iop"));
    const auto email = conn.arg(IOP_STR("iopEmail"));
    const auto password = conn.arg(IOP_STR("iopPassword"));
    if (iop && email && password) {
      logger.debug(IOP_STR("Email: "));
      logger.debugln(*email);
      this->credentialsIop = std::unique_ptr<DynamicCredential>(new (std::nothrow) DynamicCredential(*email, *password));
      iop_assert(this->credentialsIop, IOP_STR("Unable to allocate credentialsIop"));
    }

    conn.sendHeader(IOP_STR("Location"), IOP_STR("/"));
    conn.send(302, IOP_STR("text/plain"), IOP_STR(""));
  });

  this->server.onNotFound([this](iop_hal::HttpConnection &conn, iop::Log &logger) {
    IOP_TRACE();
    logger.infoln(IOP_STR("Serving form"));

    const auto needsIopAuth = !eventLoop.storage().token() && !this->credentialsIop;
    // Not needing IoP auth means the WiFi credentials we have are invalid
    const auto mustConnect = !iop::Network::isConnected() && (!eventLoop.storage().wifi() || !needsIopAuth);

    auto len = pageHTMLStart().length() + pageHTMLEnd().length() + script().length() + css().length();
    len += mustConnect ? wifiHTML().length() : wifiOverwriteHTML().length();
    len += needsIopAuth ? iopHTML().length() : iopOverwriteHTML().length();

    conn.setContentLength(len);
    conn.send(200, IOP_STR("text/html"), pageHTMLStart());

    if (mustConnect) conn.sendData(wifiHTML());
    else conn.sendData(wifiOverwriteHTML());

    if (needsIopAuth) conn.sendData(iopHTML());
    else conn.sendData(iopOverwriteHTML());

    conn.sendData(script());
    conn.sendData(css());
    conn.sendData(pageHTMLEnd());
    logger.debugln(IOP_STR("Served HTML"));
  });
}

CredentialsServer::CredentialsServer() noexcept: logger(IOP_STR("SERVER")) {}

auto CredentialsServer::setAccessPointCredentials(StaticString SSID, StaticString PSK) noexcept -> void {
  this->credentialsAccessPoint = std::unique_ptr<StaticCredential>(new (std::nothrow) StaticCredential(SSID, PSK));
  iop_assert(this->credentialsAccessPoint, IOP_STR("Unable to allocate credentialsAccessPoint"));
}

auto CredentialsServer::start() noexcept -> void {
  IOP_TRACE();
  if (!this->isServerOpen) {
    this->isServerOpen = true;
    this->logger.info(IOP_STR("Network connection: "));
    this->logger.infoln(iop::Network::isConnected());
    this->logger.infoln(IOP_STR("Valid credentials are not available"));
    this->logger.info(IOP_STR("Setting our own wifi access point: "));
    this->logger.infoln(this->credentialsAccessPoint->login);

    const auto hasWifi = eventLoop.storage().wifi() || this->credentialsWifi;
    this->logger.debug(IOP_STR("Has Wifi Creds: "));
    this->logger.debugln(hasWifi);

    const auto hasIop = eventLoop.storage().token() || this->credentialsIop;
    this->logger.debug(IOP_STR("Has IoP Creds: "));
    this->logger.debugln(hasIop);

    const auto isConnected = iop::Network::isConnected();
    iop_assert(this->credentialsAccessPoint, IOP_STR("Must configure Access Point credentials"));
    iop::wifi.enableOurAccessPoint(this->credentialsAccessPoint->login.toString(), this->credentialsAccessPoint->password.toString());

    // Some architectures force a disconnect when AP is enabled (AP_STA is fragile), so we reconnect
    if (isConnected && !iop::Network::isConnected()) {
      const auto wifi = eventLoop.storage().wifi();
      // Should have something, but there might be a race so we can't be sure
      if (wifi) {
        eventLoop.connect(iop::to_view(wifi->get().ssid), iop::to_view(wifi->get().password.get()));
      }
    }

    // Makes it a captive portal (redirects all wifi trafic to it)
    this->dnsServer.start();
    this->server.begin();

    this->logger.info(IOP_STR("Opened captive portal: "));
    this->logger.infoln(iop::wifi.ourAccessPointIp());
  }
}

auto CredentialsServer::close() noexcept -> bool {
  IOP_TRACE();
  if (this->isServerOpen) {
    this->logger.debugln(IOP_STR("Closing captive portal"));
    this->isServerOpen = false;

    this->dnsServer.close();
    this->server.close();

    iop::wifi.disableOurAccessPoint();
    return true;
  }
  return false;
}

auto CredentialsServer::serve() noexcept -> std::unique_ptr<DynamicCredential> {
  IOP_TRACE();

  if (this->credentialsWifi) {
    this->close();

    this->logger.infoln(IOP_STR("Connecting to WiFi"));
    eventLoop.connect(this->credentialsWifi->login, this->credentialsWifi->password);
    this->credentialsWifi = nullptr;
    return nullptr;
  }

  if (iop::Network::isConnected() && this->credentialsIop) {
    if (!this->close()) return std::move(this->credentialsIop);
    return nullptr;
  }

  this->start();

  this->logger.traceln(IOP_STR("Serve captive portal"));
  this->dnsServer.handleClient();
  this->server.handleClient();
  return nullptr;
}
}
