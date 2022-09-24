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
    "<html><body>\r\n"
    "  <h1><center>Hello, I'm your plantomator</center></h1>\r\n"
    "  <h4><center>If, in the future, you want to reset the "
    "configurations set here, just press the factory reset button "
    "for at least 15 seconds</center></h4>"
    "<form style='margin: 0 auto; width: 500px;' action='/submit' "
    "method='POST'>\r\n");
}

auto wifiOverwriteHTML() -> iop::StaticString {
  return IOP_STR(
    "<h3>"
    "  <center>It seems you already have your wifi credentials set, if you "
    "want to rewrite it, please set the checkbox below and fill the "
    "fields. Otherwise they will be ignored</center>"
    "</h3>\r\n"
    "<div>"
    "  <input type='checkbox' name='wifi'>"
    "  <label for='wifi'>Overwrite wifi credentials</label>"
    "</div>"
    "<div class=\"wifi\" style=\"display: none\">"
    "  <div><strong>Network name:</strong></div>"
    "  <input name='ssid' type='text' style='width:100%' />"
    "</div>\r\n"
    "<div class=\"wifi\" style=\"display: none\">"
    "  <div><strong>Password:</strong></div>"
    "  <input name='password' type='password' style='width:100%' />"
    "</div>\r\n");
}

auto wifiHTML() -> iop::StaticString {
  return IOP_STR(
    "<h3><center>"
    "Please provide your Wifi credentials, so we can connect to it."
    "</center></h3>\r\n"
    "<div><input type='hidden' value='true' name='wifi'></div>"
    "<div>"
    "  <div><strong>Network name:</strong></div>"
    "  <input name='ssid' type='text' style='width:100%' />"
    "</div>\r\n"
    "<div><div><strong>Password:</strong></div>"
    "<input name='password' type='password' style='width:100%' /></div>\r\n");
}

auto iopOverwriteHTML() -> iop::StaticString {
  return IOP_STR(
    "<h3><center>It seems you already have your Iop credentials set, if you "
    "want to rewrite it, please set the checkbox below and fill the "
    "fields. Otherwise they will be ignored</center></h3>\r\n"
    "<div>"
    "  <input type='checkbox' name='iop'>"
    "  <label for='iop'>Overwrite Iop credentials</label>"
    "</div>"
    "<div class=\"iop\" style=\"display: 'none'\">"
    "  <div><strong>Email:</strong></div>"
    "  <input name='iopEmail' type='text' style='width:100%' />"
    "</div>\r\n"
    "<div class=\"iop\" style=\"display: 'none'\">"
    "  <div><strong>Password:</strong></div>"
    "  <input name='iopPassword' type='password' style='width:100%' />"
    "</div>\r\n");
}

auto iopHTML() -> iop::StaticString {
  return IOP_STR(
    "<h3><center>Please provide your Iop credentials, so we can get an "
    "authentication token to use</center></h3>\r\n"
    "<div>"
    "  <input type='hidden' value='true' name='iop'></div>"
    "<div>"
    "  <div><strong>Email:</strong></div>"
    "  <input name='iopEmail' type='text' style='width:100%' />"
    "</div>\r\n"
    "<div>"
    "  <div><strong>Password:</strong></div>"
    "  <input name='iopPassword' type='password' style='width:100%' />"
    "</div>\r\n");
}

auto script() -> iop::StaticString {
  return IOP_STR(
    "<script type='application/javascript'>"
    "document.querySelector(\"input[name='wifi']\").addEventListener('change', ev => {"
    "  for (const el of document.getElementsByClassName('wifi')) {"
    "    if (ev.currentTarget.checked) {"
    "      el.style.display = 'block';"
    "    } else {"
    "      el.style.display = 'none';"
    "    }"
    "  }"
    "});"
    "document.querySelector(\"input[name='iop']\").addEventListener('change', ev => {"
    "  for (const el of document.getElementsByClassName('iop')) {"
    "    if (ev.currentTarget.checked) {"
    "      el.style.display = 'block';"
    "    } else {"
    "      el.style.display = 'none';"
    "    }"
    "  }"
    "});"
    "</script>");
}

auto pageHTMLEnd() -> iop::StaticString {
  return IOP_STR(
    "<br>\r\n"
    "<input type='submit' value='Submit' />\r\n"
    "</form></body></html>");
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
      iop_assert(this->credentialsWifi, IOP_STR("Unable to allocate credentialsIop"));
    }

    conn.sendHeader(IOP_STR("Location"), IOP_STR("/"));
    conn.send(302, IOP_STR("text/plain"), IOP_STR(""));
  });

  this->server.onNotFound([](iop_hal::HttpConnection &conn, iop::Log &logger) {
    IOP_TRACE();
    logger.infoln(IOP_STR("Serving captive portal"));

    const auto mustConnect = !iop::Network::isConnected();
    const auto needsIopAuth = !eventLoop.storage().token();

    auto len = pageHTMLStart().length() + pageHTMLEnd().length() + script().length();
    len += mustConnect ? wifiHTML().length() : wifiOverwriteHTML().length();
    len += needsIopAuth ? iopHTML().length() : iopOverwriteHTML().length();

    conn.setContentLength(len);
    conn.send(200, IOP_STR("text/html"), pageHTMLStart());

    if (mustConnect) conn.sendData(wifiHTML());
    else conn.sendData(wifiOverwriteHTML());

    if (needsIopAuth) conn.sendData(iopHTML());
    else conn.sendData(iopOverwriteHTML());

    conn.sendData(script());
    conn.sendData(pageHTMLEnd());
    logger.debugln(IOP_STR("Served HTML"));
  });
}

CredentialsServer::CredentialsServer(const iop::LogLevel logLevel) noexcept: logger(logLevel, IOP_STR("SERVER")) {}

auto CredentialsServer::setAccessPointCredentials(StaticString SSID, StaticString PSK) noexcept -> void {
  this->credentialsAccessPoint = std::unique_ptr<StaticCredential>(new (std::nothrow) StaticCredential(SSID, PSK));
  iop_assert(this->credentialsAccessPoint, IOP_STR("Unable to allocate credentialsAccessPoint"))
}

auto CredentialsServer::start() noexcept -> void {
  IOP_TRACE();
  if (!this->isServerOpen) {
    this->isServerOpen = true;
    this->logger.infoln(IOP_STR("Setting our own wifi access point"));

    iop_assert(this->credentialsAccessPoint, IOP_STR("Must configure Access Point credentials"));
    iop::wifi.enableOurAccessPoint(this->credentialsAccessPoint->login.toString(), this->credentialsAccessPoint->password.toString());

    // Makes it a captive portal (redirects all wifi trafic to it)
    this->dnsServer.start();
    this->server.begin();

    this->logger.info(IOP_STR("Opened captive portal: "));
    this->logger.infoln(iop::wifi.ourAccessPointIp());
  }
}

auto CredentialsServer::close() noexcept -> void {
  IOP_TRACE();
  if (this->isServerOpen) {
    this->logger.debugln(IOP_STR("Closing captive portal"));
    this->isServerOpen = false;

    this->dnsServer.close();
    this->server.close();

    iop::wifi.disableOurAccessPoint();
  }
}

auto CredentialsServer::serve(Api &api) noexcept -> std::unique_ptr<AuthToken> {
  this->start();

  if (this->handleWifiCreds()) return nullptr;

  auto token = this->handleIopCreds(api);
  if (token) return token;

  this->handleClient();
  return nullptr;
}

auto CredentialsServer::handleClient() noexcept -> void {
  this->logger.traceln(IOP_STR("Serve captive portal"));
  this->dnsServer.handleClient();
  this->server.handleClient();
}

auto CredentialsServer::handleWifiCreds() noexcept -> bool {
  if (this->credentialsWifi) {
    this->logger.infoln(IOP_STR("Connecting to WiFi"));
    eventLoop.connect(this->credentialsWifi->login, this->credentialsWifi->password);
    this->credentialsWifi = nullptr;
    return true;
  }
  return false;
}

auto CredentialsServer::handleIopCreds(Api &api) noexcept -> std::unique_ptr<AuthToken> {
  if (iop::Network::isConnected() && this->credentialsIop) {
    this->logger.infoln(IOP_STR("Connecting to IoP"));
    auto tok = eventLoop.authenticate(this->credentialsIop->login, this->credentialsIop->password, api);
    this->credentialsIop = nullptr;
    if (tok)
      return tok;
  }
  return nullptr;
}
}