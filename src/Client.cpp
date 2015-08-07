
#include "Client.hpp"
#include "tcp/IPC.hpp"
#include <onions-common/Common.hpp>
#include <onions-common/Log.hpp>
#include <onions-common/Config.hpp>
#include <onions-common/Constants.hpp>
#include <onions-common/Utils.hpp>



void Client::listenForDomains(ushort socksPort)
{
  auto addr = Config::getMirror()[0];
  socks_ = SocksClient::getCircuitTo(addr["ip"].asString(),
                                     static_cast<short>(addr["port"].asInt()),
                                     socksPort);
  if (!socks_)
    Log::get().error("Unable to connect!");

  IPC ipc(Const::IPC_PORT);
  ipc.start();
}



std::string Client::resolve(const std::string& torDomain)
{
  try
  {
    std::string domain = torDomain;

    while (Utils::strEndsWith(domain, ".tor"))
    {
      // check cache first
      auto iterator = cache_.find(domain);
      if (iterator == cache_.end())
      {
        Log::get().notice("Sending \"" + domain + "\" to name server...");

        auto received = socks_->sendReceive("domainQuery", domain);
        if (received["type"] == "error")
        {
          Log::get().warn(received["value"].asString());
          return "<Response_Error>";
        }

        Log::get().notice("Received Record response.");
        auto dest = Common::getDestination(
            Common::parseRecord(received["value"].asString()), domain);

        cache_[domain] = dest;
        domain = dest;
      }
      else
        domain = iterator->second;  // retrieve from cache
    }

    if (domain.length() != 22 || !Utils::strEndsWith(domain, ".onion"))
    {
      Log::get().warn("\"" + domain + "\" is not a HS address!");
      return "<Invalid_Result>";
    }

    return domain;
  }
  catch (std::runtime_error& re)
  {
    Log::get().error(re.what());
  }

  return "<General_Error>";
}
