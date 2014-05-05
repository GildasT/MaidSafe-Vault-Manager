/*  Copyright 2014 MaidSafe.net limited

    This MaidSafe Software is licensed to you under (1) the MaidSafe.net Commercial License,
    version 1.0 or later, or (2) The General Public License (GPL), version 3, depending on which
    licence you accepted on initial access to the Software (the "Licences").

    By contributing code to the MaidSafe Software, or to this project generally, you agree to be
    bound by the terms of the MaidSafe Contributor Agreement, version 1.0, found in the root
    directory of this project at LICENSE, COPYING and CONTRIBUTOR respectively and also
    available at: http://www.maidsafe.net/licenses

    Unless required by applicable law or agreed to in writing, the MaidSafe Software distributed
    under the GPL Licence is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS
    OF ANY KIND, either express or implied.

    See the Licences for the specific language governing permissions and limitations relating to
    use of the MaidSafe Software.                                                                 */

#ifndef MAIDSAFE_VAULT_MANAGER_CLIENT_INTERFACE_H_
#define MAIDSAFE_VAULT_MANAGER_CLIENT_INTERFACE_H_

#include <condition_variable>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

#include "boost/asio/ip/udp.hpp"
#include "boost/date_time/posix_time/posix_time_duration.hpp"
#include "boost/filesystem/path.hpp"
#include "boost/signals2/connection.hpp"
#include "boost/signals2/signal.hpp"

#include "maidsafe/common/asio_service.h"
#include "maidsafe/common/rsa.h"

#include "maidsafe/passport/types.h"

namespace maidsafe {

namespace vault_manager {

class LocalTcpTransport;

typedef boost::signals2::signal<void(const std::string&)> OnNewVersionAvailable;
typedef std::pair<std::string, Port> EndPoint;

class ClientInterface {
 public:
  ClientInterface(std::function<void(const std::string&)> on_new_version_available_slot);
  ~ClientInterface();

  std::vector<boost::asio::ip::udp::endpoint> BootstrapEndpoints();

  // Blocking call to start a vault with the specified identity information and account name.
  bool StartVault(const passport::Pmid& pmid, const passport::Maid::Name& account_name,
                  const boost::filesystem::path& chunkstore);

  // Blocking call to stop the vault with the specified identity. For authentication, provide data
  // signed wth the vault's private key.
  bool StopVault(const asymm::PlainText& data, const asymm::Signature& signature,
                 const Identity& identity);

  // Blocking call to retrieve the latest bootstrap nodes from the VaultManager.
  bool GetBootstrapNodes(std::vector<boost::asio::ip::udp::endpoint>& bootstrap_endpoints);

#ifdef TESTING
  static void SetTestEnvironmentVariables(
      Port test_vault_manager_port, boost::filesystem::path test_env_root_dir,
      boost::filesystem::path path_to_vault,
      std::vector<boost::asio::ip::udp::endpoint> bootstrap_ips);
#endif

 private:
  typedef std::shared_ptr<LocalTcpTransport> TransportPtr;
  enum State {
    kInitialising,
    kVerified,
    kFailed
  };

  ClientInterface(const ClientInterface&);
  ClientInterface& operator=(const ClientInterface&);
  bool FindNextAcceptingPort(TransportPtr requesting_transport);
  bool ConnectToVaultManager(std::string& path_to_new_installer);
  void HandleRegisterResponse(const std::string& message, Port vault_manager_port,
                              std::mutex& mutex, std::condition_variable& condition_variable,
                              State& state, std::string& path_to_new_installer);
  template <typename ResponseType>
  void HandleStartStopVaultResponse(const std::string& message,
                                    const std::function<void(bool)>& callback);
  boost::posix_time::time_duration SetOrGetUpdateInterval(
      const boost::posix_time::time_duration& update_interval);
  void HandleUpdateIntervalResponse(
      const std::string& message,
      const std::function<void(boost::posix_time::time_duration)>& callback);
  void HandleReceivedRequest(const std::string& message, Port peer_port);
  void HandleNewVersionAvailable(const std::string& request, std::string& response);
  void HandleVaultJoinConfirmation(const std::string& request, std::string& response);
  void HandleBootstrapResponse(const std::string& message,
                               std::vector<boost::asio::ip::udp::endpoint>& bootstrap_endpoints,
                               std::function<void(bool)> callback);

  Port vault_manager_port_, local_port_;
  OnNewVersionAvailable on_new_version_available_;
  std::vector<boost::asio::ip::udp::endpoint> bootstrap_nodes_;
  std::map<passport::Pmid::Name, bool> joining_vaults_;
  std::mutex joining_vaults_mutex_;
  std::condition_variable joining_vaults_conditional_;
  AsioService asio_service_;
  TransportPtr receiving_transport_;
};

}  // namespace vault_manager

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_MANAGER_CLIENT_INTERFACE_H_