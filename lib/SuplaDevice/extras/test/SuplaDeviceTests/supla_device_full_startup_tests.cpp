/*
 Copyright (C) AC SOFTWARE SP. Z O.O.

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <arduino_mock.h>
#include <srpc_mock.h>
#include <timer_mock.h>
#include <SuplaDevice.h>
#include <supla/clock/clock.h>
#include <supla/storage/storage.h>
#include <element_mock.h>
#include <board_mock.h>
#include "supla/protocol/supla_srpc.h"
#include <network_client_mock.h>
#include <config_mock.h>
#include <supla/sensor/therm_hygro_press_meter.h>

using ::testing::Return;
using ::testing::_;
using ::testing::DoAll;
using ::testing::Assign;
using ::testing::ReturnPointee;

class SuplaDeviceTests : public ::testing::Test {
  protected:
    virtual void SetUp() {
      Supla::Channel::lastCommunicationTimeMs = 0;
      memset(&(Supla::Channel::reg_dev), 0, sizeof(Supla::Channel::reg_dev));
    }
    virtual void TearDown() {
      Supla::Channel::lastCommunicationTimeMs = 0;
      memset(&(Supla::Channel::reg_dev), 0, sizeof(Supla::Channel::reg_dev));
    }

};

class SimpleTime : public TimeInterface {
  public:
    SimpleTime() : value(0) {}

    virtual uint64_t millis() override {
      return value;
    }

    void advance(int advanceMs) {
      value += advanceMs;
    }

    uint64_t value;
};

class StorageMock2: public Supla::Storage {
 public:
  MOCK_METHOD(bool, init, (), (override));
  MOCK_METHOD(bool, prepareState, (bool), (override));
  MOCK_METHOD(bool, finalizeSaveState, (), (override));
  MOCK_METHOD(void, commit, (), (override));
  MOCK_METHOD(int, readStorage, (unsigned int, unsigned char *, int, bool), (override));
  MOCK_METHOD(int, writeStorage, (unsigned int, const unsigned char *, int), (override));

};

class NetworkMock : public Supla::Network {
 public:
  NetworkMock() : Supla::Network(nullptr){};
  MOCK_METHOD(void, setup, (), (override));
  MOCK_METHOD(void, disable, (), (override));

  MOCK_METHOD(bool, isReady, (), (override));
  MOCK_METHOD(bool, iterate, (), (override));
};

class SuplaDeviceTestsFullStartup : public SuplaDeviceTests {
  protected:
    SrpcMock srpc;
    NetworkMock net;
    TimerMock timer;
    SimpleTime time;
    SuplaDeviceClass sd;
    ElementMock el1;
    ElementMock el2;
    BoardMock board;
    NetworkClientMock *client = nullptr;

    virtual void SetUp() {
      client = new NetworkClientMock;  // it will be destroyed in
                                       // Supla::Protocol::SuplaSrpc
      SuplaDeviceTests::SetUp();

      int dummy;

      EXPECT_CALL(el1, onInit());
      EXPECT_CALL(el2, onInit());

      EXPECT_CALL(timer, initTimers());
      EXPECT_CALL(srpc, srpc_params_init(_));
      EXPECT_CALL(srpc, srpc_init(_)).WillOnce(Return(&dummy));
      EXPECT_CALL(srpc, srpc_set_proto_version(&dummy, 16));

      char GUID[SUPLA_GUID_SIZE] = {1};
      char AUTHKEY[SUPLA_AUTHKEY_SIZE] = {2};
      EXPECT_TRUE(sd.begin(GUID, "supla.rulez", "superman@supla.org", AUTHKEY));
      EXPECT_EQ(sd.getCurrentStatus(), STATUS_INITIALIZED);
    }

    virtual void TearDown() {
      SuplaDeviceTests::TearDown();
    }
};

class SuplaDeviceElementWithSecondaryChannel : public SuplaDeviceTestsFullStartup {
  protected:
    Supla::Sensor::ThermHygroPressMeter thpm;
};

class SuplaDeviceTestsFullStartupManual : public SuplaDeviceTests {
  protected:
    SrpcMock srpc;
    NetworkMock net;
    TimerMock timer;
    SimpleTime time;
    SuplaDeviceClass sd;
    ElementMock el1;
    ElementMock el2;
    BoardMock board;
    NetworkClientMock *client = nullptr;

    virtual void SetUp() {
      client = new NetworkClientMock;  // it will be destroyed in
                                       // Supla::Protocol::SuplaSrpc
      SuplaDeviceTests::SetUp();
    }

    virtual void TearDown() {
      SuplaDeviceTests::TearDown();
    }
};

using ::testing::AtLeast;

TEST_F(SuplaDeviceTestsFullStartup, NoNetworkShouldCallSetupAgain) {
  EXPECT_CALL(net, isReady()).WillRepeatedly(Return(false));
  EXPECT_CALL(net, setup()).Times(3);
  EXPECT_CALL(*client, stop()).Times(2);
  EXPECT_CALL(el1, iterateAlways()).Times(AtLeast(1));
  EXPECT_CALL(el2, iterateAlways()).Times(AtLeast(1));

  for (int i = 0; i < 131*10; i++) {
    sd.iterate();
    time.advance(100);
  }
  EXPECT_EQ(sd.getCurrentStatus(), STATUS_NETWORK_DISCONNECTED);
}

TEST_F(SuplaDeviceTestsFullStartup, FailedConnectionShouldSetupNetworkAgain) {
  EXPECT_CALL(net, isReady()).WillRepeatedly(Return(true));
  EXPECT_CALL(*client, connected()).WillRepeatedly(Return(false));
  EXPECT_CALL(*client, connectImp(_, _)).WillRepeatedly(Return(0));
  EXPECT_CALL(*client, stop()).Times(AtLeast(1));
  EXPECT_CALL(net, iterate()).WillRepeatedly(Return(true));

  EXPECT_CALL(net, setup()).Times(2);
  EXPECT_CALL(el1, iterateAlways()).Times(AtLeast(1));
  EXPECT_CALL(el2, iterateAlways()).Times(AtLeast(1));

  for (int i = 0; i < 61*10; i++) {
    sd.iterate();
    time.advance(100);
  }
  EXPECT_EQ(sd.getCurrentStatus(), STATUS_SERVER_DISCONNECTED);
}

TEST_F(SuplaDeviceTestsFullStartup, SrpcFailureShouldCallDisconnect) {
  EXPECT_CALL(net, isReady()).WillRepeatedly(Return(true));
  EXPECT_CALL(*client, connected()).WillOnce(Return(false)).WillRepeatedly(Return(false));
  EXPECT_CALL(*client, connectImp(_, _)).WillRepeatedly(Return(1));
  EXPECT_CALL(net, setup()).Times(1);
  EXPECT_CALL(net, iterate()).Times(1);
  EXPECT_CALL(srpc, srpc_iterate(_)).WillOnce(Return(SUPLA_RESULT_FALSE));

  EXPECT_CALL(*client, stop()).Times(1);

  EXPECT_CALL(el1, iterateAlways()).Times(AtLeast(1));
  EXPECT_CALL(el2, iterateAlways()).Times(AtLeast(1));

  sd.iterate();
  EXPECT_EQ(sd.getCurrentStatus(), STATUS_ITERATE_FAIL);
}

TEST_F(SuplaDeviceTestsFullStartup, NoReplyForDeviceRegistrationShoudResetConnection) {
  bool isConnected = false;
  EXPECT_CALL(net, isReady()).WillRepeatedly(Return(true));
  EXPECT_CALL(*client, connected()).WillRepeatedly(ReturnPointee(&isConnected));
  EXPECT_CALL(*client, connectImp(_, _)).WillRepeatedly(DoAll(Assign(&isConnected, true), Return(1)));

  EXPECT_CALL(net, setup()).Times(1);
  EXPECT_CALL(net, iterate()).Times(AtLeast(1));
  EXPECT_CALL(srpc, srpc_iterate(_)).WillRepeatedly(Return(SUPLA_RESULT_TRUE));

  EXPECT_CALL(el1, iterateAlways()).Times(AtLeast(1));
  EXPECT_CALL(el2, iterateAlways()).Times(AtLeast(1));

  EXPECT_CALL(*client, stop()).WillOnce(Assign(&isConnected, false));

  EXPECT_CALL(srpc, srpc_ds_async_registerdevice_e(_, _)).Times(2);

  for (int i = 0; i < 11*10; i++) {
    sd.iterate();
    time.advance(100);
  }
  EXPECT_EQ(sd.getCurrentStatus(), STATUS_SERVER_DISCONNECTED);
  for (int i = 0; i < 2*10; i++) {
    sd.iterate();
    time.advance(100);
  }
  EXPECT_EQ(sd.getCurrentStatus(), STATUS_REGISTER_IN_PROGRESS);
}


TEST_F(SuplaDeviceTestsFullStartup, SuccessfulStartup) {
  bool isConnected = false;
  EXPECT_CALL(net, isReady()).WillRepeatedly(Return(true));
  EXPECT_CALL(*client, connected()).WillRepeatedly(ReturnPointee(&isConnected));
  EXPECT_CALL(*client, connectImp(_, _)).WillRepeatedly(DoAll(Assign(&isConnected, true), Return(1)));

  EXPECT_CALL(net, setup()).Times(1);
  EXPECT_CALL(net, iterate()).Times(AtLeast(1));
  EXPECT_CALL(srpc, srpc_iterate(_)).WillRepeatedly(Return(SUPLA_RESULT_TRUE));

  EXPECT_CALL(el1, iterateAlways()).Times(35);
  EXPECT_CALL(el2, iterateAlways()).Times(35);

  EXPECT_CALL(el1, onRegistered(_));
  EXPECT_CALL(el2, onRegistered(_));

  EXPECT_CALL(srpc, srpc_ds_async_registerdevice_e(_, _)).Times(1);
  EXPECT_CALL(srpc, srpc_dcs_async_set_activity_timeout(_, _)).Times(1);
  EXPECT_CALL(srpc, srpc_dcs_async_ping_server(_)).Times(2);

//  EXPECT_CALL(net, ping(_)).WillRepeatedly(Return(true));
  EXPECT_CALL(el1, iterateConnected(_)).Times(30).WillRepeatedly(Return(true));
  EXPECT_CALL(el2, iterateConnected(_)).Times(30).WillRepeatedly(Return(true));

  EXPECT_EQ(sd.getCurrentStatus(), STATUS_INITIALIZED);
  for (int i = 0; i < 5; i++) {
    sd.iterate();
    time.advance(1000);
  }
  EXPECT_EQ(sd.getCurrentStatus(), STATUS_REGISTER_IN_PROGRESS);

  TSD_SuplaRegisterDeviceResult register_device_result{};
  register_device_result.result_code = SUPLA_RESULTCODE_TRUE;
  register_device_result.activity_timeout = 45;
  register_device_result.version = 16;
  register_device_result.version_min = 1;

  auto srpcLayer = sd.getSrpcLayer();
  srpcLayer->onRegisterResult(&register_device_result);
  time.advance(100);

  EXPECT_EQ(sd.getCurrentStatus(), STATUS_REGISTERED_AND_READY);

  for (int i = 0; i < 30; i++) {
    sd.iterate();
    time.advance(1000);
  }

  EXPECT_EQ(sd.getCurrentStatus(), STATUS_REGISTERED_AND_READY);
}

TEST_F(SuplaDeviceTestsFullStartup, NoNetworkShouldCallSetupAgainAndResetDev) {
  EXPECT_CALL(net, isReady()).WillRepeatedly(Return(false));
  EXPECT_CALL(net, setup()).Times(2);
  EXPECT_CALL(*client, stop()).Times(1);
  EXPECT_CALL(el1, iterateAlways()).Times(AtLeast(1));
  EXPECT_CALL(el2, iterateAlways()).Times(AtLeast(1));
  // In tests we can't reset board, so this method will be called few times
  EXPECT_CALL(board, deviceSoftwareReset()).Times(AtLeast(1));
  EXPECT_CALL(el1, onSaveState()).Times(AtLeast(1));
  EXPECT_CALL(el2, onSaveState()).Times(AtLeast(1));
  sd.setAutomaticResetOnConnectionProblem(100);

  for (int i = 0; i < 102*10; i++) {
    sd.iterate();
    time.advance(100);
  }

  EXPECT_EQ(sd.getCurrentStatus(), STATUS_SOFTWARE_RESET);
}

TEST_F(SuplaDeviceTestsFullStartupManual,
    SslDisabledNoConfig) {
  net.setSSLEnabled(false);
  EXPECT_EQ(Supla::Storage::ConfigInstance(), nullptr);
  int dummy = 0;
  sd.setActivityTimeout(45);

  EXPECT_CALL(el1, onLoadConfig()).Times(0);
  EXPECT_CALL(el2, onLoadConfig()).Times(0);
  EXPECT_CALL(el1, onInit()).Times(1);
  EXPECT_CALL(el2, onInit()).Times(1);
  EXPECT_CALL(el1, onRegistered(_));
  EXPECT_CALL(el2, onRegistered(_));

  EXPECT_CALL(timer, initTimers());
  EXPECT_CALL(srpc, srpc_params_init(_));
  EXPECT_CALL(srpc, srpc_init(_)).WillOnce(Return(&dummy));
  EXPECT_CALL(srpc, srpc_set_proto_version(&dummy, 18));

  char GUID[SUPLA_GUID_SIZE] = {1};
  char AUTHKEY[SUPLA_AUTHKEY_SIZE] = {2};
  EXPECT_TRUE(sd.begin(GUID, "supla.rulez", "superman@supla.org", AUTHKEY, 18));
  EXPECT_EQ(sd.getCurrentStatus(), STATUS_INITIALIZED);

  EXPECT_CALL(net, isReady()).WillRepeatedly(Return(true));
  EXPECT_CALL(net, setup()).Times(1);
  EXPECT_CALL(net, iterate()).Times(AtLeast(1));
  EXPECT_CALL(srpc, srpc_iterate(_)).WillRepeatedly(Return(SUPLA_RESULT_TRUE));
  EXPECT_CALL(*client, stop()).Times(0);

  EXPECT_CALL(*client, connected()).WillOnce(Return(false))
    .WillRepeatedly(Return(true));
  // SSL disabled, port should be 2015
  EXPECT_CALL(*client, connectImp(_, 2015)).WillRepeatedly(Return(1));
  EXPECT_CALL(srpc, srpc_ds_async_registerdevice_e(_, _)).Times(1);

  EXPECT_CALL(el1, iterateAlways()).Times(AtLeast(1));
  EXPECT_CALL(el2, iterateAlways()).Times(AtLeast(1));
  EXPECT_CALL(el1, iterateConnected(_)).Times(AtLeast(1))
        .WillOnce(Return(false))
        .WillRepeatedly(Return(true));
  EXPECT_CALL(el2, iterateConnected(_)).Times(AtLeast(1));

  EXPECT_CALL(board, deviceSoftwareReset()).Times(0);
//  EXPECT_CALL(el1, onSaveState()).Times(AtLeast(1));
//  EXPECT_CALL(el2, onSaveState()).Times(AtLeast(1));

  EXPECT_EQ(sd.getCurrentStatus(), STATUS_INITIALIZED);
  for (int i = 0; i < 5; i++) {
    sd.iterate();
    time.advance(100);
  }
  EXPECT_EQ(sd.getCurrentStatus(), STATUS_REGISTER_IN_PROGRESS);

  TSD_SuplaRegisterDeviceResult register_device_result{};
  register_device_result.result_code = SUPLA_RESULTCODE_TRUE;
  register_device_result.activity_timeout = 45;
  register_device_result.version = 20;
  register_device_result.version_min = 1;

  auto srpcLayer = sd.getSrpcLayer();
  srpcLayer->onRegisterResult(&register_device_result);
  time.advance(100);

  EXPECT_EQ(sd.getCurrentStatus(), STATUS_REGISTERED_AND_READY);

  for (int i = 0; i < 10; i++) {
    sd.iterate();
    time.advance(100);
  }

  EXPECT_EQ(sd.getCurrentStatus(), STATUS_REGISTERED_AND_READY);
  EXPECT_EQ(client->getRootCACert(), nullptr);
}

TEST_F(SuplaDeviceTestsFullStartupManual,
    SslDisabledCAConfiguredNoConfig) {
  net.setSSLEnabled(false);
  EXPECT_EQ(Supla::Storage::ConfigInstance(), nullptr);
  int dummy = 0;
  sd.setActivityTimeout(45);
  const char myCA1[] = "test CA1";
  const char myCA2[] = "test CA2";
  sd.setSuplaCACert(myCA1);
  sd.setSupla3rdPartyCACert(myCA2);

  EXPECT_CALL(el1, onLoadConfig()).Times(0);
  EXPECT_CALL(el2, onLoadConfig()).Times(0);
  EXPECT_CALL(el1, onInit()).Times(1);
  EXPECT_CALL(el2, onInit()).Times(1);
  EXPECT_CALL(el1, onRegistered(_));
  EXPECT_CALL(el2, onRegistered(_));

  EXPECT_CALL(timer, initTimers());
  EXPECT_CALL(srpc, srpc_params_init(_));
  EXPECT_CALL(srpc, srpc_init(_)).WillOnce(Return(&dummy));
  EXPECT_CALL(srpc, srpc_set_proto_version(&dummy, 18));

  char GUID[SUPLA_GUID_SIZE] = {1};
  char AUTHKEY[SUPLA_AUTHKEY_SIZE] = {2};
  EXPECT_TRUE(sd.begin(GUID, "supla.rulez", "superman@supla.org", AUTHKEY, 18));
  EXPECT_EQ(sd.getCurrentStatus(), STATUS_INITIALIZED);

  EXPECT_CALL(net, isReady()).WillRepeatedly(Return(true));
  EXPECT_CALL(net, setup()).Times(1);
  EXPECT_CALL(net, iterate()).Times(AtLeast(1));
  EXPECT_CALL(srpc, srpc_iterate(_)).WillRepeatedly(Return(SUPLA_RESULT_TRUE));
  EXPECT_CALL(*client, stop()).Times(0);

  EXPECT_CALL(*client, connected()).WillOnce(Return(false))
    .WillRepeatedly(Return(true));
  // SSL disabled, port should be 2015
  EXPECT_CALL(*client, connectImp(_, 2015)).WillRepeatedly(Return(1));
  EXPECT_CALL(srpc, srpc_ds_async_registerdevice_e(_, _)).Times(1);

  EXPECT_CALL(el1, iterateAlways()).Times(AtLeast(1));
  EXPECT_CALL(el2, iterateAlways()).Times(AtLeast(1));
  EXPECT_CALL(el1, iterateConnected(_)).Times(AtLeast(1))
        .WillOnce(Return(false))
        .WillRepeatedly(Return(true));
  EXPECT_CALL(el2, iterateConnected(_)).Times(AtLeast(1));

  EXPECT_CALL(board, deviceSoftwareReset()).Times(0);
//  EXPECT_CALL(el1, onSaveState()).Times(AtLeast(1));
//  EXPECT_CALL(el2, onSaveState()).Times(AtLeast(1));

  EXPECT_EQ(sd.getCurrentStatus(), STATUS_INITIALIZED);
  for (int i = 0; i < 5; i++) {
    sd.iterate();
    time.advance(100);
  }
  EXPECT_EQ(sd.getCurrentStatus(), STATUS_REGISTER_IN_PROGRESS);

  TSD_SuplaRegisterDeviceResult register_device_result{};
  register_device_result.result_code = SUPLA_RESULTCODE_TRUE;
  register_device_result.activity_timeout = 45;
  register_device_result.version = 20;
  register_device_result.version_min = 1;

  auto srpcLayer = sd.getSrpcLayer();
  srpcLayer->onRegisterResult(&register_device_result);
  time.advance(100);

  EXPECT_EQ(sd.getCurrentStatus(), STATUS_REGISTERED_AND_READY);

  for (int i = 0; i < 10; i++) {
    sd.iterate();
    time.advance(100);
  }

  EXPECT_EQ(sd.getCurrentStatus(), STATUS_REGISTERED_AND_READY);
  EXPECT_EQ(client->getRootCACert(), nullptr);
}

TEST_F(SuplaDeviceTestsFullStartupManual,
    SslEnabledNoCANoConfig) {
  net.setSSLEnabled(true);
  EXPECT_EQ(Supla::Storage::ConfigInstance(), nullptr);
  int dummy = 0;
  sd.setActivityTimeout(45);

  EXPECT_CALL(el1, onLoadConfig()).Times(0);
  EXPECT_CALL(el2, onLoadConfig()).Times(0);
  EXPECT_CALL(el1, onInit()).Times(1);
  EXPECT_CALL(el2, onInit()).Times(1);
  EXPECT_CALL(el1, onRegistered(_));
  EXPECT_CALL(el2, onRegistered(_));

  EXPECT_CALL(timer, initTimers());
  EXPECT_CALL(srpc, srpc_params_init(_));
  EXPECT_CALL(srpc, srpc_init(_)).WillOnce(Return(&dummy));
  EXPECT_CALL(srpc, srpc_set_proto_version(&dummy, 18));

  char GUID[SUPLA_GUID_SIZE] = {1};
  char AUTHKEY[SUPLA_AUTHKEY_SIZE] = {2};
  EXPECT_TRUE(sd.begin(GUID, "supla.rulez", "superman@supla.org", AUTHKEY, 18));
  EXPECT_EQ(sd.getCurrentStatus(), STATUS_INITIALIZED);

  EXPECT_CALL(net, isReady()).WillRepeatedly(Return(true));
  EXPECT_CALL(net, setup()).Times(1);
  EXPECT_CALL(net, iterate()).Times(AtLeast(1));
  EXPECT_CALL(srpc, srpc_iterate(_)).WillRepeatedly(Return(SUPLA_RESULT_TRUE));
  EXPECT_CALL(*client, stop()).Times(0);

  EXPECT_CALL(*client, connected()).WillOnce(Return(false))
    .WillRepeatedly(Return(true));
  // Port should be 2016
  EXPECT_CALL(*client, connectImp(_, 2016)).WillRepeatedly(Return(1));
  EXPECT_CALL(srpc, srpc_ds_async_registerdevice_e(_, _)).Times(1);

  EXPECT_CALL(el1, iterateAlways()).Times(AtLeast(1));
  EXPECT_CALL(el2, iterateAlways()).Times(AtLeast(1));
  EXPECT_CALL(el1, iterateConnected(_)).Times(AtLeast(1))
        .WillOnce(Return(false))
        .WillRepeatedly(Return(true));
  EXPECT_CALL(el2, iterateConnected(_)).Times(AtLeast(1));

  EXPECT_CALL(board, deviceSoftwareReset()).Times(0);
//  EXPECT_CALL(el1, onSaveState()).Times(AtLeast(1));
//  EXPECT_CALL(el2, onSaveState()).Times(AtLeast(1));

  EXPECT_EQ(sd.getCurrentStatus(), STATUS_INITIALIZED);
  for (int i = 0; i < 5; i++) {
    sd.iterate();
    time.advance(100);
  }
  EXPECT_EQ(sd.getCurrentStatus(), STATUS_REGISTER_IN_PROGRESS);

  TSD_SuplaRegisterDeviceResult register_device_result{};
  register_device_result.result_code = SUPLA_RESULTCODE_TRUE;
  register_device_result.activity_timeout = 45;
  register_device_result.version = 20;
  register_device_result.version_min = 1;

  auto srpcLayer = sd.getSrpcLayer();
  srpcLayer->onRegisterResult(&register_device_result);
  time.advance(100);

  EXPECT_EQ(sd.getCurrentStatus(), STATUS_REGISTERED_AND_READY);

  for (int i = 0; i < 10; i++) {
    sd.iterate();
    time.advance(100);
  }

  EXPECT_EQ(sd.getCurrentStatus(), STATUS_REGISTERED_AND_READY);
  EXPECT_EQ(client->getRootCACert(), nullptr);
}

TEST_F(SuplaDeviceTestsFullStartupManual,
    SslEnabledSuplaCANoConfig) {
  net.setSSLEnabled(true);
  const char myCA1[] = "test CA1";
  const char myCA2[] = "test CA2";
  sd.setSuplaCACert(myCA1);
  sd.setSupla3rdPartyCACert(myCA2);

  EXPECT_EQ(Supla::Storage::ConfigInstance(), nullptr);
  int dummy = 0;
  sd.setActivityTimeout(45);

  EXPECT_CALL(el1, onLoadConfig()).Times(0);
  EXPECT_CALL(el2, onLoadConfig()).Times(0);
  EXPECT_CALL(el1, onInit()).Times(1);
  EXPECT_CALL(el2, onInit()).Times(1);
  EXPECT_CALL(el1, onRegistered(_));
  EXPECT_CALL(el2, onRegistered(_));

  EXPECT_CALL(timer, initTimers());
  EXPECT_CALL(srpc, srpc_params_init(_));
  EXPECT_CALL(srpc, srpc_init(_)).WillOnce(Return(&dummy));
  EXPECT_CALL(srpc, srpc_set_proto_version(&dummy, 18));

  char GUID[SUPLA_GUID_SIZE] = {1};
  char AUTHKEY[SUPLA_AUTHKEY_SIZE] = {2};
  EXPECT_TRUE(sd.begin(GUID, "ok.supla.org", "superman@supla.org", AUTHKEY, 18));
  EXPECT_EQ(sd.getCurrentStatus(), STATUS_INITIALIZED);

  EXPECT_CALL(net, isReady()).WillRepeatedly(Return(true));
  EXPECT_CALL(net, setup()).Times(1);
  EXPECT_CALL(net, iterate()).Times(AtLeast(1));
  EXPECT_CALL(srpc, srpc_iterate(_)).WillRepeatedly(Return(SUPLA_RESULT_TRUE));
  EXPECT_CALL(*client, stop()).Times(0);

  EXPECT_CALL(*client, connected()).WillOnce(Return(false))
    .WillRepeatedly(Return(true));
  EXPECT_CALL(*client, connectImp(_, 2016)).WillRepeatedly(Return(1));
  EXPECT_CALL(srpc, srpc_ds_async_registerdevice_e(_, _)).Times(1);

  EXPECT_CALL(el1, iterateAlways()).Times(AtLeast(1));
  EXPECT_CALL(el2, iterateAlways()).Times(AtLeast(1));
  EXPECT_CALL(el1, iterateConnected(_)).Times(AtLeast(1))
        .WillOnce(Return(false))
        .WillRepeatedly(Return(true));
  EXPECT_CALL(el2, iterateConnected(_)).Times(AtLeast(1));

  EXPECT_CALL(board, deviceSoftwareReset()).Times(0);
//  EXPECT_CALL(el1, onSaveState()).Times(AtLeast(1));
//  EXPECT_CALL(el2, onSaveState()).Times(AtLeast(1));

  EXPECT_EQ(sd.getCurrentStatus(), STATUS_INITIALIZED);
  for (int i = 0; i < 5; i++) {
    sd.iterate();
    time.advance(100);
  }
  EXPECT_EQ(sd.getCurrentStatus(), STATUS_REGISTER_IN_PROGRESS);

  TSD_SuplaRegisterDeviceResult register_device_result{};
  register_device_result.result_code = SUPLA_RESULTCODE_TRUE;
  register_device_result.activity_timeout = 45;
  register_device_result.version = 20;
  register_device_result.version_min = 1;

  auto srpcLayer = sd.getSrpcLayer();
  srpcLayer->onRegisterResult(&register_device_result);
  time.advance(100);

  EXPECT_EQ(sd.getCurrentStatus(), STATUS_REGISTERED_AND_READY);

  for (int i = 0; i < 10; i++) {
    sd.iterate();
    time.advance(100);
  }

  EXPECT_EQ(sd.getCurrentStatus(), STATUS_REGISTERED_AND_READY);
  EXPECT_EQ(client->getRootCACert(), myCA1);
}

TEST_F(SuplaDeviceTestsFullStartupManual,
    SslEnabledSuplaCAPrivateServerNoConfig) {
  net.setSSLEnabled(true);
  const char myCA1[] = "test CA1";
  const char myCA2[] = "test CA2";
  sd.setSuplaCACert(myCA1);
  sd.setSupla3rdPartyCACert(myCA2);

  EXPECT_EQ(Supla::Storage::ConfigInstance(), nullptr);
  int dummy = 0;
  sd.setActivityTimeout(45);

  EXPECT_CALL(el1, onLoadConfig()).Times(0);
  EXPECT_CALL(el2, onLoadConfig()).Times(0);
  EXPECT_CALL(el1, onInit()).Times(1);
  EXPECT_CALL(el2, onInit()).Times(1);
  EXPECT_CALL(el1, onRegistered(_));
  EXPECT_CALL(el2, onRegistered(_));

  EXPECT_CALL(timer, initTimers());
  EXPECT_CALL(srpc, srpc_params_init(_));
  EXPECT_CALL(srpc, srpc_init(_)).WillOnce(Return(&dummy));
  EXPECT_CALL(srpc, srpc_set_proto_version(&dummy, 18));

  char GUID[SUPLA_GUID_SIZE] = {1};
  char AUTHKEY[SUPLA_AUTHKEY_SIZE] = {2};
  EXPECT_TRUE(sd.begin(GUID, "supla.rulez", "superman@supla.priv", AUTHKEY, 18));
  EXPECT_EQ(sd.getCurrentStatus(), STATUS_INITIALIZED);

  EXPECT_CALL(net, isReady()).WillRepeatedly(Return(true));
  EXPECT_CALL(net, setup()).Times(1);
  EXPECT_CALL(net, iterate()).Times(AtLeast(1));
  EXPECT_CALL(srpc, srpc_iterate(_)).WillRepeatedly(Return(SUPLA_RESULT_TRUE));
  EXPECT_CALL(*client, stop()).Times(0);

  EXPECT_CALL(*client, connected()).WillOnce(Return(false))
    .WillRepeatedly(Return(true));
  EXPECT_CALL(*client, connectImp(_, 2016)).WillRepeatedly(Return(1));
  EXPECT_CALL(srpc, srpc_ds_async_registerdevice_e(_, _)).Times(1);

  EXPECT_CALL(el1, iterateAlways()).Times(AtLeast(1));
  EXPECT_CALL(el2, iterateAlways()).Times(AtLeast(1));
  EXPECT_CALL(el1, iterateConnected(_)).Times(AtLeast(1))
        .WillOnce(Return(false))
        .WillRepeatedly(Return(true));
  EXPECT_CALL(el2, iterateConnected(_)).Times(AtLeast(1));

  EXPECT_CALL(board, deviceSoftwareReset()).Times(0);
//  EXPECT_CALL(el1, onSaveState()).Times(AtLeast(1));
//  EXPECT_CALL(el2, onSaveState()).Times(AtLeast(1));

  EXPECT_EQ(sd.getCurrentStatus(), STATUS_INITIALIZED);
  for (int i = 0; i < 5; i++) {
    sd.iterate();
    time.advance(100);
  }
  EXPECT_EQ(sd.getCurrentStatus(), STATUS_REGISTER_IN_PROGRESS);

  TSD_SuplaRegisterDeviceResult register_device_result{};
  register_device_result.result_code = SUPLA_RESULTCODE_TRUE;
  register_device_result.activity_timeout = 45;
  register_device_result.version = 20;
  register_device_result.version_min = 1;

  auto srpcLayer = sd.getSrpcLayer();
  srpcLayer->onRegisterResult(&register_device_result);
  time.advance(100);

  EXPECT_EQ(sd.getCurrentStatus(), STATUS_REGISTERED_AND_READY);

  for (int i = 0; i < 10; i++) {
    sd.iterate();
    time.advance(100);
  }

  EXPECT_EQ(sd.getCurrentStatus(), STATUS_REGISTERED_AND_READY);
  EXPECT_EQ(client->getRootCACert(), myCA2);
}

TEST_F(SuplaDeviceTestsFullStartupManual,
    SslEnabledOnlyOnceCASetNoConfig) {
  net.setSSLEnabled(true);
  const char myCA1[] = "test CA1";
  const char myCA2[] = "test CA2";
  sd.setSuplaCACert(myCA1);
//  sd.setSupla3rdPartyCACert(myCA2);

  EXPECT_EQ(Supla::Storage::ConfigInstance(), nullptr);
  int dummy = 0;
  sd.setActivityTimeout(45);

  EXPECT_CALL(el1, onLoadConfig()).Times(0);
  EXPECT_CALL(el2, onLoadConfig()).Times(0);
  EXPECT_CALL(el1, onInit()).Times(1);
  EXPECT_CALL(el2, onInit()).Times(1);
  EXPECT_CALL(el1, onRegistered(_));
  EXPECT_CALL(el2, onRegistered(_));

  EXPECT_CALL(timer, initTimers());
  EXPECT_CALL(srpc, srpc_params_init(_));
  EXPECT_CALL(srpc, srpc_init(_)).WillOnce(Return(&dummy));
  EXPECT_CALL(srpc, srpc_set_proto_version(&dummy, 18));

  char GUID[SUPLA_GUID_SIZE] = {1};
  char AUTHKEY[SUPLA_AUTHKEY_SIZE] = {2};
  EXPECT_TRUE(sd.begin(GUID, "supla.rulez", "superman@supla.priv", AUTHKEY, 18));
  EXPECT_EQ(sd.getCurrentStatus(), STATUS_INITIALIZED);

  EXPECT_CALL(net, isReady()).WillRepeatedly(Return(true));
  EXPECT_CALL(net, setup()).Times(1);
  EXPECT_CALL(net, iterate()).Times(AtLeast(1));
  EXPECT_CALL(srpc, srpc_iterate(_)).WillRepeatedly(Return(SUPLA_RESULT_TRUE));
  EXPECT_CALL(*client, stop()).Times(0);

  EXPECT_CALL(*client, connected()).WillOnce(Return(false))
    .WillRepeatedly(Return(true));
  EXPECT_CALL(*client, connectImp(_, 2016)).WillRepeatedly(Return(1));
  EXPECT_CALL(srpc, srpc_ds_async_registerdevice_e(_, _)).Times(1);

  EXPECT_CALL(el1, iterateAlways()).Times(AtLeast(1));
  EXPECT_CALL(el2, iterateAlways()).Times(AtLeast(1));
  EXPECT_CALL(el1, iterateConnected(_)).Times(AtLeast(1))
        .WillOnce(Return(false))
        .WillRepeatedly(Return(true));
  EXPECT_CALL(el2, iterateConnected(_)).Times(AtLeast(1));

  EXPECT_CALL(board, deviceSoftwareReset()).Times(0);
//  EXPECT_CALL(el1, onSaveState()).Times(AtLeast(1));
//  EXPECT_CALL(el2, onSaveState()).Times(AtLeast(1));

  EXPECT_EQ(sd.getCurrentStatus(), STATUS_INITIALIZED);
  for (int i = 0; i < 5; i++) {
    sd.iterate();
    time.advance(100);
  }
  EXPECT_EQ(sd.getCurrentStatus(), STATUS_REGISTER_IN_PROGRESS);

  TSD_SuplaRegisterDeviceResult register_device_result{};
  register_device_result.result_code = SUPLA_RESULTCODE_TRUE;
  register_device_result.activity_timeout = 45;
  register_device_result.version = 20;
  register_device_result.version_min = 1;

  auto srpcLayer = sd.getSrpcLayer();
  srpcLayer->onRegisterResult(&register_device_result);
  time.advance(100);

  EXPECT_EQ(sd.getCurrentStatus(), STATUS_REGISTERED_AND_READY);

  for (int i = 0; i < 10; i++) {
    sd.iterate();
    time.advance(100);
  }

  EXPECT_EQ(sd.getCurrentStatus(), STATUS_REGISTERED_AND_READY);
  EXPECT_EQ(client->getRootCACert(), myCA1);
}

TEST_F(SuplaDeviceTestsFullStartupManual,
    SslEnabledOnlyOnceCASetv2NoConfig) {
  net.setSSLEnabled(true);
  const char myCA1[] = "test CA1";
  const char myCA2[] = "test CA2";
//  sd.setSuplaCACert(myCA1);
  sd.setSupla3rdPartyCACert(myCA2);

  EXPECT_EQ(Supla::Storage::ConfigInstance(), nullptr);
  int dummy = 0;
  sd.setActivityTimeout(45);

  EXPECT_CALL(el1, onLoadConfig()).Times(0);
  EXPECT_CALL(el2, onLoadConfig()).Times(0);
  EXPECT_CALL(el1, onInit()).Times(1);
  EXPECT_CALL(el2, onInit()).Times(1);
  EXPECT_CALL(el1, onRegistered(_));
  EXPECT_CALL(el2, onRegistered(_));

  EXPECT_CALL(timer, initTimers());
  EXPECT_CALL(srpc, srpc_params_init(_));
  EXPECT_CALL(srpc, srpc_init(_)).WillOnce(Return(&dummy));
  EXPECT_CALL(srpc, srpc_set_proto_version(&dummy, 18));

  char GUID[SUPLA_GUID_SIZE] = {1};
  char AUTHKEY[SUPLA_AUTHKEY_SIZE] = {2};
  EXPECT_TRUE(sd.begin(GUID, "test.supla.org", "superman@supla.org", AUTHKEY, 18));
  EXPECT_EQ(sd.getCurrentStatus(), STATUS_INITIALIZED);

  EXPECT_CALL(net, isReady()).WillRepeatedly(Return(true));
  EXPECT_CALL(net, setup()).Times(1);
  EXPECT_CALL(net, iterate()).Times(AtLeast(1));
  EXPECT_CALL(srpc, srpc_iterate(_)).WillRepeatedly(Return(SUPLA_RESULT_TRUE));
  EXPECT_CALL(*client, stop()).Times(0);

  EXPECT_CALL(*client, connected()).WillOnce(Return(false))
    .WillRepeatedly(Return(true));
  EXPECT_CALL(*client, connectImp(_, 2016)).WillRepeatedly(Return(1));
  EXPECT_CALL(srpc, srpc_ds_async_registerdevice_e(_, _)).Times(1);

  EXPECT_CALL(el1, iterateAlways()).Times(AtLeast(1));
  EXPECT_CALL(el2, iterateAlways()).Times(AtLeast(1));
  EXPECT_CALL(el1, iterateConnected(_)).Times(AtLeast(1))
        .WillOnce(Return(false))
        .WillRepeatedly(Return(true));
  EXPECT_CALL(el2, iterateConnected(_)).Times(AtLeast(1));

  EXPECT_CALL(board, deviceSoftwareReset()).Times(0);
//  EXPECT_CALL(el1, onSaveState()).Times(AtLeast(1));
//  EXPECT_CALL(el2, onSaveState()).Times(AtLeast(1));

  EXPECT_EQ(sd.getCurrentStatus(), STATUS_INITIALIZED);
  for (int i = 0; i < 5; i++) {
    sd.iterate();
    time.advance(100);
  }
  EXPECT_EQ(sd.getCurrentStatus(), STATUS_REGISTER_IN_PROGRESS);

  TSD_SuplaRegisterDeviceResult register_device_result{};
  register_device_result.result_code = SUPLA_RESULTCODE_TRUE;
  register_device_result.activity_timeout = 45;
  register_device_result.version = 20;
  register_device_result.version_min = 1;

  auto srpcLayer = sd.getSrpcLayer();
  srpcLayer->onRegisterResult(&register_device_result);
  time.advance(100);

  EXPECT_EQ(sd.getCurrentStatus(), STATUS_REGISTERED_AND_READY);

  for (int i = 0; i < 10; i++) {
    sd.iterate();
    time.advance(100);
  }

  EXPECT_EQ(sd.getCurrentStatus(), STATUS_REGISTERED_AND_READY);
  EXPECT_EQ(client->getRootCACert(), myCA2);
}

TEST_F(SuplaDeviceTestsFullStartupManual,
    WithConfigSslDisabled) {
  ConfigMock cfg;
  const char myCA1[] = "test CA1";
  const char myCA2[] = "test CA2";
  sd.setSuplaCACert(myCA1);
  sd.setSupla3rdPartyCACert(myCA2);

  EXPECT_NE(Supla::Storage::ConfigInstance(), nullptr);
  int dummy = 0;
  sd.setActivityTimeout(45);


  EXPECT_CALL(cfg, init()).WillOnce(Return(true));
  EXPECT_CALL(cfg, isConfigModeSupported()).WillRepeatedly(Return(true));
  EXPECT_CALL(cfg, getDeviceName(_)).WillRepeatedly(Return(false));
  EXPECT_CALL(cfg, getGUID(_)).WillRepeatedly([] (char *guid) {
      char GUID[SUPLA_GUID_SIZE] = {1};
      memcpy(guid, GUID, SUPLA_GUID_SIZE);
      return true;
    });
  EXPECT_CALL(cfg, getAuthKey(_)).WillRepeatedly([] (char *auth) {
      char AUTHKEY[SUPLA_AUTHKEY_SIZE] = {2};
      memcpy(auth, AUTHKEY, SUPLA_AUTHKEY_SIZE);
      return true;
    });
  EXPECT_CALL(cfg, getDeviceMode())
    .WillRepeatedly(Return(Supla::DEVICE_MODE_NOT_SET));
  EXPECT_CALL(cfg, getSuplaServer(_)).WillRepeatedly([] (char *server) {
      char temp[] = "mega.supla.org";
      memcpy(server, temp, strlen(temp));
      return true;
    });
  EXPECT_CALL(cfg, getEmail(_)).WillRepeatedly([] (char *mail) {
      char temp[] = "ceo@supla.org";
      memcpy(mail, temp, strlen(temp));
      return true;
    });
  // ssl disabled in config is realized by setting port to 2015
  EXPECT_CALL(cfg, getSuplaServerPort()).WillRepeatedly(Return(2015));
  EXPECT_CALL(cfg, saveIfNeeded()).Times(AtLeast(1));
  EXPECT_CALL(cfg, getUInt8(_, _)).WillRepeatedly([] (const char *key,
        uint8_t *buf) {
      if (strcmp(key, "security_level")) {
        *buf = 0;
        return true;
      }
      return false;
    });


  EXPECT_CALL(cfg, isSuplaCommProtocolEnabled()).WillRepeatedly(Return(true));


  EXPECT_CALL(el1, onLoadConfig()).Times(1);
  EXPECT_CALL(el2, onLoadConfig()).Times(1);
  EXPECT_CALL(el1, onInit()).Times(1);
  EXPECT_CALL(el2, onInit()).Times(1);
  EXPECT_CALL(el1, onRegistered(_));
  EXPECT_CALL(el2, onRegistered(_));

  EXPECT_CALL(timer, initTimers());
  EXPECT_CALL(srpc, srpc_params_init(_));
  EXPECT_CALL(srpc, srpc_init(_)).WillOnce(Return(&dummy));
  EXPECT_CALL(srpc, srpc_set_proto_version(&dummy, 18));

  EXPECT_TRUE(sd.begin(18));
  EXPECT_EQ(sd.getCurrentStatus(), STATUS_INITIALIZED);

  EXPECT_CALL(net, isReady()).WillRepeatedly(Return(true));
  EXPECT_CALL(net, setup()).Times(1);
  EXPECT_CALL(net, iterate()).Times(AtLeast(1));
  EXPECT_CALL(srpc, srpc_iterate(_)).WillRepeatedly(Return(SUPLA_RESULT_TRUE));
  EXPECT_CALL(*client, stop()).Times(0);

  EXPECT_CALL(*client, connected()).WillOnce(Return(false))
    .WillRepeatedly(Return(true));
  EXPECT_CALL(*client, connectImp(_, 2015)).WillRepeatedly(Return(1));
  EXPECT_CALL(srpc, srpc_ds_async_registerdevice_e(_, _)).Times(1);

  EXPECT_CALL(el1, iterateAlways()).Times(AtLeast(1));
  EXPECT_CALL(el2, iterateAlways()).Times(AtLeast(1));
  EXPECT_CALL(el1, iterateConnected(_)).Times(AtLeast(1))
        .WillOnce(Return(false))
        .WillRepeatedly(Return(true));
  EXPECT_CALL(el2, iterateConnected(_)).Times(AtLeast(1));

  EXPECT_CALL(board, deviceSoftwareReset()).Times(0);
//  EXPECT_CALL(el1, onSaveState()).Times(AtLeast(1));
//  EXPECT_CALL(el2, onSaveState()).Times(AtLeast(1));

  EXPECT_EQ(sd.getCurrentStatus(), STATUS_INITIALIZED);
  for (int i = 0; i < 5; i++) {
    sd.iterate();
    time.advance(100);
  }
  EXPECT_EQ(sd.getCurrentStatus(), STATUS_REGISTER_IN_PROGRESS);

  TSD_SuplaRegisterDeviceResult register_device_result{};
  register_device_result.result_code = SUPLA_RESULTCODE_TRUE;
  register_device_result.activity_timeout = 45;
  register_device_result.version = 20;
  register_device_result.version_min = 1;

  auto srpcLayer = sd.getSrpcLayer();
  srpcLayer->onRegisterResult(&register_device_result);
  time.advance(100);

  EXPECT_EQ(sd.getCurrentStatus(), STATUS_REGISTERED_AND_READY);

  for (int i = 0; i < 10; i++) {
    sd.iterate();
    time.advance(100);
  }

  EXPECT_EQ(sd.getCurrentStatus(), STATUS_REGISTERED_AND_READY);
  EXPECT_EQ(client->getRootCACert(), nullptr);
}

TEST_F(SuplaDeviceTestsFullStartupManual,
    WithConfigSslEnabledManually) {
  ConfigMock cfg;
  const char myCA1[] = "test CA1";
  const char myCA2[] = "test CA2";
  sd.setSuplaCACert(myCA1);
  sd.setSupla3rdPartyCACert(myCA2);

  EXPECT_NE(Supla::Storage::ConfigInstance(), nullptr);
  int dummy = 0;
  sd.setActivityTimeout(45);


  EXPECT_CALL(cfg, init()).WillOnce(Return(true));
  EXPECT_CALL(cfg, isConfigModeSupported()).WillRepeatedly(Return(true));
  EXPECT_CALL(cfg, getDeviceName(_)).WillRepeatedly(Return(false));
  EXPECT_CALL(cfg, getGUID(_)).WillRepeatedly([] (char *guid) {
      char GUID[SUPLA_GUID_SIZE] = {1};
      memcpy(guid, GUID, SUPLA_GUID_SIZE);
      return true;
    });
  EXPECT_CALL(cfg, getAuthKey(_)).WillRepeatedly([] (char *auth) {
      char AUTHKEY[SUPLA_AUTHKEY_SIZE] = {2};
      memcpy(auth, AUTHKEY, SUPLA_AUTHKEY_SIZE);
      return true;
    });
  EXPECT_CALL(cfg, getDeviceMode())
    .WillRepeatedly(Return(Supla::DEVICE_MODE_NOT_SET));
  EXPECT_CALL(cfg, getSuplaServer(_)).WillRepeatedly([] (char *server) {
      char temp[] = "mega.supla.org";
      memcpy(server, temp, strlen(temp));
      return true;
    });
  EXPECT_CALL(cfg, getEmail(_)).WillRepeatedly([] (char *mail) {
      char temp[] = "ceo@supla.org";
      memcpy(mail, temp, strlen(temp));
      return true;
    });
  // ssl enabled in config is realized by setting port to 2016
  EXPECT_CALL(cfg, getSuplaServerPort()).WillRepeatedly(Return(2016));
  EXPECT_CALL(cfg, saveIfNeeded()).Times(AtLeast(1));
  EXPECT_CALL(cfg, getUInt8(_, _)).WillRepeatedly([] (const char *key,
        uint8_t *buf) {
      if (strcmp(key, "security_level")) {
        *buf = 0;
        return true;
      }
      return false;
    });


  EXPECT_CALL(cfg, isSuplaCommProtocolEnabled()).WillRepeatedly(Return(true));


  EXPECT_CALL(el1, onLoadConfig()).Times(1);
  EXPECT_CALL(el2, onLoadConfig()).Times(1);
  EXPECT_CALL(el1, onInit()).Times(1);
  EXPECT_CALL(el2, onInit()).Times(1);
  EXPECT_CALL(el1, onRegistered(_));
  EXPECT_CALL(el2, onRegistered(_));

  EXPECT_CALL(timer, initTimers());
  EXPECT_CALL(srpc, srpc_params_init(_));
  EXPECT_CALL(srpc, srpc_init(_)).WillOnce(Return(&dummy));
  EXPECT_CALL(srpc, srpc_set_proto_version(&dummy, 18));

  EXPECT_TRUE(sd.begin(18));
  EXPECT_EQ(sd.getCurrentStatus(), STATUS_INITIALIZED);

  EXPECT_CALL(net, isReady()).WillRepeatedly(Return(true));
  EXPECT_CALL(net, setup()).Times(1);
  EXPECT_CALL(net, iterate()).Times(AtLeast(1));
  EXPECT_CALL(srpc, srpc_iterate(_)).WillRepeatedly(Return(SUPLA_RESULT_TRUE));
  EXPECT_CALL(*client, stop()).Times(0);

  EXPECT_CALL(*client, connected()).WillOnce(Return(false))
    .WillRepeatedly(Return(true));
  EXPECT_CALL(*client, connectImp(_, 2016)).WillRepeatedly(Return(1));
  EXPECT_CALL(srpc, srpc_ds_async_registerdevice_e(_, _)).Times(1);

  EXPECT_CALL(el1, iterateAlways()).Times(AtLeast(1));
  EXPECT_CALL(el2, iterateAlways()).Times(AtLeast(1));
  EXPECT_CALL(el1, iterateConnected(_)).Times(AtLeast(1))
        .WillOnce(Return(false))
        .WillRepeatedly(Return(true));
  EXPECT_CALL(el2, iterateConnected(_)).Times(AtLeast(1));

  EXPECT_CALL(board, deviceSoftwareReset()).Times(0);
//  EXPECT_CALL(el1, onSaveState()).Times(AtLeast(1));
//  EXPECT_CALL(el2, onSaveState()).Times(AtLeast(1));

  EXPECT_EQ(sd.getCurrentStatus(), STATUS_INITIALIZED);
  for (int i = 0; i < 5; i++) {
    sd.iterate();
    time.advance(100);
  }
  EXPECT_EQ(sd.getCurrentStatus(), STATUS_REGISTER_IN_PROGRESS);

  TSD_SuplaRegisterDeviceResult register_device_result{};
  register_device_result.result_code = SUPLA_RESULTCODE_TRUE;
  register_device_result.activity_timeout = 45;
  register_device_result.version = 20;
  register_device_result.version_min = 1;

  auto srpcLayer = sd.getSrpcLayer();
  srpcLayer->onRegisterResult(&register_device_result);
  time.advance(100);

  EXPECT_EQ(sd.getCurrentStatus(), STATUS_REGISTERED_AND_READY);

  for (int i = 0; i < 10; i++) {
    sd.iterate();
    time.advance(100);
  }

  EXPECT_EQ(sd.getCurrentStatus(), STATUS_REGISTERED_AND_READY);
  EXPECT_EQ(client->getRootCACert(), myCA1);
}

TEST_F(SuplaDeviceTestsFullStartupManual,
    WithConfigSslEnabled) {
  ConfigMock cfg;
  const char myCA1[] = "test CA1";
  const char myCA2[] = "test CA2";
  sd.setSuplaCACert(myCA1);
  sd.setSupla3rdPartyCACert(myCA2);

  EXPECT_NE(Supla::Storage::ConfigInstance(), nullptr);
  int dummy = 0;
  sd.setActivityTimeout(45);


  EXPECT_CALL(cfg, init()).WillOnce(Return(true));
  EXPECT_CALL(cfg, isConfigModeSupported()).WillRepeatedly(Return(true));
  EXPECT_CALL(cfg, getDeviceName(_)).WillRepeatedly(Return(false));
  EXPECT_CALL(cfg, getGUID(_)).WillRepeatedly([] (char *guid) {
      char GUID[SUPLA_GUID_SIZE] = {1};
      memcpy(guid, GUID, SUPLA_GUID_SIZE);
      return true;
    });
  EXPECT_CALL(cfg, getAuthKey(_)).WillRepeatedly([] (char *auth) {
      char AUTHKEY[SUPLA_AUTHKEY_SIZE] = {2};
      memcpy(auth, AUTHKEY, SUPLA_AUTHKEY_SIZE);
      return true;
    });
  EXPECT_CALL(cfg, getDeviceMode())
    .WillRepeatedly(Return(Supla::DEVICE_MODE_NOT_SET));
  EXPECT_CALL(cfg, getSuplaServer(_)).WillRepeatedly([] (char *server) {
      char temp[] = "mega.supla.org";
      memcpy(server, temp, strlen(temp));
      return true;
    });
  EXPECT_CALL(cfg, getEmail(_)).WillRepeatedly([] (char *mail) {
      char temp[] = "ceo@supla.org";
      memcpy(mail, temp, strlen(temp));
      return true;
    });
  // ssl enabled in config is realized by setting port to -1 (auto/default)
  EXPECT_CALL(cfg, getSuplaServerPort()).WillRepeatedly(Return(-1));
  EXPECT_CALL(cfg, saveIfNeeded()).Times(AtLeast(1));
  EXPECT_CALL(cfg, getUInt8(_, _)).WillRepeatedly([] (const char *key,
        uint8_t *buf) {
      if (strcmp(key, "security_level")) {
        *buf = 0;
        return true;
      }
      return false;
    });


  EXPECT_CALL(cfg, isSuplaCommProtocolEnabled()).WillRepeatedly(Return(true));


  EXPECT_CALL(el1, onLoadConfig()).Times(1);
  EXPECT_CALL(el2, onLoadConfig()).Times(1);
  EXPECT_CALL(el1, onInit()).Times(1);
  EXPECT_CALL(el2, onInit()).Times(1);
  EXPECT_CALL(el1, onRegistered(_));
  EXPECT_CALL(el2, onRegistered(_));

  EXPECT_CALL(timer, initTimers());
  EXPECT_CALL(srpc, srpc_params_init(_));
  EXPECT_CALL(srpc, srpc_init(_)).WillOnce(Return(&dummy));
  EXPECT_CALL(srpc, srpc_set_proto_version(&dummy, 18));

  EXPECT_TRUE(sd.begin(18));
  EXPECT_EQ(sd.getCurrentStatus(), STATUS_INITIALIZED);

  EXPECT_CALL(net, isReady()).WillRepeatedly(Return(true));
  EXPECT_CALL(net, setup()).Times(1);
  EXPECT_CALL(net, iterate()).Times(AtLeast(1));
  EXPECT_CALL(srpc, srpc_iterate(_)).WillRepeatedly(Return(SUPLA_RESULT_TRUE));
  EXPECT_CALL(*client, stop()).Times(0);

  EXPECT_CALL(*client, connected()).WillOnce(Return(false))
    .WillRepeatedly(Return(true));
  EXPECT_CALL(*client, connectImp(_, 2016)).WillRepeatedly(Return(1));
  EXPECT_CALL(srpc, srpc_ds_async_registerdevice_e(_, _)).Times(1);

  EXPECT_CALL(el1, iterateAlways()).Times(AtLeast(1));
  EXPECT_CALL(el2, iterateAlways()).Times(AtLeast(1));
  EXPECT_CALL(el1, iterateConnected(_)).Times(AtLeast(1))
        .WillOnce(Return(false))
        .WillRepeatedly(Return(true));
  EXPECT_CALL(el2, iterateConnected(_)).Times(AtLeast(1));

  EXPECT_CALL(board, deviceSoftwareReset()).Times(0);
//  EXPECT_CALL(el1, onSaveState()).Times(AtLeast(1));
//  EXPECT_CALL(el2, onSaveState()).Times(AtLeast(1));

  EXPECT_EQ(sd.getCurrentStatus(), STATUS_INITIALIZED);
  for (int i = 0; i < 5; i++) {
    sd.iterate();
    time.advance(100);
  }
  EXPECT_EQ(sd.getCurrentStatus(), STATUS_REGISTER_IN_PROGRESS);

  TSD_SuplaRegisterDeviceResult register_device_result{};
  register_device_result.result_code = SUPLA_RESULTCODE_TRUE;
  register_device_result.activity_timeout = 45;
  register_device_result.version = 20;
  register_device_result.version_min = 1;

  auto srpcLayer = sd.getSrpcLayer();
  srpcLayer->onRegisterResult(&register_device_result);
  time.advance(100);

  EXPECT_EQ(sd.getCurrentStatus(), STATUS_REGISTERED_AND_READY);

  for (int i = 0; i < 10; i++) {
    sd.iterate();
    time.advance(100);
  }

  EXPECT_EQ(sd.getCurrentStatus(), STATUS_REGISTERED_AND_READY);
  EXPECT_EQ(client->getRootCACert(), myCA1);
}

TEST_F(SuplaDeviceTestsFullStartupManual,
    WithConfigSslEnabledPrivateServer) {
  ConfigMock cfg;
  const char myCA1[] = "test CA1";
  const char myCA2[] = "test CA2";
  sd.setSuplaCACert(myCA1);
  sd.setSupla3rdPartyCACert(myCA2);

  EXPECT_NE(Supla::Storage::ConfigInstance(), nullptr);
  int dummy = 0;
  sd.setActivityTimeout(45);


  EXPECT_CALL(cfg, init()).WillOnce(Return(true));
  EXPECT_CALL(cfg, isConfigModeSupported()).WillRepeatedly(Return(true));
  EXPECT_CALL(cfg, getDeviceName(_)).WillRepeatedly(Return(false));
  EXPECT_CALL(cfg, getGUID(_)).WillRepeatedly([] (char *guid) {
      char GUID[SUPLA_GUID_SIZE] = {1};
      memcpy(guid, GUID, SUPLA_GUID_SIZE);
      return true;
    });
  EXPECT_CALL(cfg, getAuthKey(_)).WillRepeatedly([] (char *auth) {
      char AUTHKEY[SUPLA_AUTHKEY_SIZE] = {2};
      memcpy(auth, AUTHKEY, SUPLA_AUTHKEY_SIZE);
      return true;
    });
  EXPECT_CALL(cfg, getDeviceMode())
    .WillRepeatedly(Return(Supla::DEVICE_MODE_NOT_SET));
  EXPECT_CALL(cfg, getSuplaServer(_)).WillRepeatedly([] (char *server) {
      char temp[] = "mega.supla.priv";
      memcpy(server, temp, strlen(temp));
      return true;
    });
  EXPECT_CALL(cfg, getEmail(_)).WillRepeatedly([] (char *mail) {
      char temp[] = "ceo@supla.org";
      memcpy(mail, temp, strlen(temp));
      return true;
    });
  // ssl enabled in config is realized by setting port to -1 (auto/default)
  EXPECT_CALL(cfg, getSuplaServerPort()).WillRepeatedly(Return(-1));
  EXPECT_CALL(cfg, saveIfNeeded()).Times(AtLeast(1));
  EXPECT_CALL(cfg, getUInt8(_, _)).WillRepeatedly([] (const char *key,
        uint8_t *buf) {
      if (strcmp(key, "security_level")) {
        *buf = 0;
        return true;
      }
      return false;
    });


  EXPECT_CALL(cfg, isSuplaCommProtocolEnabled()).WillRepeatedly(Return(true));


  EXPECT_CALL(el1, onLoadConfig()).Times(1);
  EXPECT_CALL(el2, onLoadConfig()).Times(1);
  EXPECT_CALL(el1, onInit()).Times(1);
  EXPECT_CALL(el2, onInit()).Times(1);
  EXPECT_CALL(el1, onRegistered(_));
  EXPECT_CALL(el2, onRegistered(_));

  EXPECT_CALL(timer, initTimers());
  EXPECT_CALL(srpc, srpc_params_init(_));
  EXPECT_CALL(srpc, srpc_init(_)).WillOnce(Return(&dummy));
  EXPECT_CALL(srpc, srpc_set_proto_version(&dummy, 18));

  EXPECT_TRUE(sd.begin(18));
  EXPECT_EQ(sd.getCurrentStatus(), STATUS_INITIALIZED);

  EXPECT_CALL(net, isReady()).WillRepeatedly(Return(true));
  EXPECT_CALL(net, setup()).Times(1);
  EXPECT_CALL(net, iterate()).Times(AtLeast(1));
  EXPECT_CALL(srpc, srpc_iterate(_)).WillRepeatedly(Return(SUPLA_RESULT_TRUE));
  EXPECT_CALL(*client, stop()).Times(0);

  EXPECT_CALL(*client, connected()).WillOnce(Return(false))
    .WillRepeatedly(Return(true));
  EXPECT_CALL(*client, connectImp(_, 2016)).WillRepeatedly(Return(1));
  EXPECT_CALL(srpc, srpc_ds_async_registerdevice_e(_, _)).Times(1);

  EXPECT_CALL(el1, iterateAlways()).Times(AtLeast(1));
  EXPECT_CALL(el2, iterateAlways()).Times(AtLeast(1));
  EXPECT_CALL(el1, iterateConnected(_)).Times(AtLeast(1))
        .WillOnce(Return(false))
        .WillRepeatedly(Return(true));
  EXPECT_CALL(el2, iterateConnected(_)).Times(AtLeast(1));

  EXPECT_CALL(board, deviceSoftwareReset()).Times(0);
//  EXPECT_CALL(el1, onSaveState()).Times(AtLeast(1));
//  EXPECT_CALL(el2, onSaveState()).Times(AtLeast(1));

  EXPECT_EQ(sd.getCurrentStatus(), STATUS_INITIALIZED);
  for (int i = 0; i < 5; i++) {
    sd.iterate();
    time.advance(100);
  }
  EXPECT_EQ(sd.getCurrentStatus(), STATUS_REGISTER_IN_PROGRESS);

  TSD_SuplaRegisterDeviceResult register_device_result{};
  register_device_result.result_code = SUPLA_RESULTCODE_TRUE;
  register_device_result.activity_timeout = 45;
  register_device_result.version = 20;
  register_device_result.version_min = 1;

  auto srpcLayer = sd.getSrpcLayer();
  srpcLayer->onRegisterResult(&register_device_result);
  time.advance(100);

  EXPECT_EQ(sd.getCurrentStatus(), STATUS_REGISTERED_AND_READY);

  for (int i = 0; i < 10; i++) {
    sd.iterate();
    time.advance(100);
  }

  EXPECT_EQ(sd.getCurrentStatus(), STATUS_REGISTERED_AND_READY);
  EXPECT_EQ(client->getRootCACert(), myCA2);
}

TEST_F(SuplaDeviceTestsFullStartupManual,
    WithConfigSslEnabledCustomCA) {
  ConfigMock cfg;
  const char myCA1[] = "test CA1";
  const char myCA2[] = "test CA2";
  sd.setSuplaCACert(myCA1);
  sd.setSupla3rdPartyCACert(myCA2);

  EXPECT_NE(Supla::Storage::ConfigInstance(), nullptr);
  int dummy = 0;
  sd.setActivityTimeout(45);


  EXPECT_CALL(cfg, init()).WillOnce(Return(true));
  EXPECT_CALL(cfg, isConfigModeSupported()).WillRepeatedly(Return(true));
  EXPECT_CALL(cfg, getDeviceName(_)).WillRepeatedly(Return(false));
  EXPECT_CALL(cfg, getGUID(_)).WillRepeatedly([] (char *guid) {
      char GUID[SUPLA_GUID_SIZE] = {1};
      memcpy(guid, GUID, SUPLA_GUID_SIZE);
      return true;
    });
  EXPECT_CALL(cfg, getAuthKey(_)).WillRepeatedly([] (char *auth) {
      char AUTHKEY[SUPLA_AUTHKEY_SIZE] = {2};
      memcpy(auth, AUTHKEY, SUPLA_AUTHKEY_SIZE);
      return true;
    });
  EXPECT_CALL(cfg, getDeviceMode())
    .WillRepeatedly(Return(Supla::DEVICE_MODE_NOT_SET));
  EXPECT_CALL(cfg, getSuplaServer(_)).WillRepeatedly([] (char *server) {
      char temp[] = "mega.supla.org";
      memcpy(server, temp, strlen(temp));
      return true;
    });
  EXPECT_CALL(cfg, getEmail(_)).WillRepeatedly([] (char *mail) {
      char temp[] = "ceo@supla.org";
      memcpy(mail, temp, strlen(temp));
      return true;
    });
  // ssl enabled in config is realized by setting port to -1 (auto/default)
  EXPECT_CALL(cfg, getSuplaServerPort()).WillRepeatedly(Return(-1));
  EXPECT_CALL(cfg, saveIfNeeded()).Times(AtLeast(1));
  EXPECT_CALL(cfg, getUInt8(_, _)).WillRepeatedly([] (const char *key,
        uint8_t *buf) {
      if (strcmp(key, "security_level") == 0) {
        *buf = 1;
        return true;
      }
      EXPECT_TRUE(false);
      return false;
    });
  EXPECT_CALL(cfg, getCustomCASize())
    .WillRepeatedly(Return(strlen("custom ca")));
  EXPECT_CALL(cfg, getCustomCA(_, _)).WillRepeatedly(
    [] (char *ca, int maxSize) {
      char temp[] = "custom ca";
      EXPECT_GE(maxSize, strlen(temp));
      memcpy(ca, temp, strlen(temp));
      return true;
    });

  EXPECT_CALL(cfg, isSuplaCommProtocolEnabled()).WillRepeatedly(Return(true));

  EXPECT_CALL(el1, onLoadConfig()).Times(1);
  EXPECT_CALL(el2, onLoadConfig()).Times(1);
  EXPECT_CALL(el1, onInit()).Times(1);
  EXPECT_CALL(el2, onInit()).Times(1);
  EXPECT_CALL(el1, onRegistered(_));
  EXPECT_CALL(el2, onRegistered(_));

  EXPECT_CALL(timer, initTimers());
  EXPECT_CALL(srpc, srpc_params_init(_));
  EXPECT_CALL(srpc, srpc_init(_)).WillOnce(Return(&dummy));
  EXPECT_CALL(srpc, srpc_set_proto_version(&dummy, 18));

  EXPECT_TRUE(sd.begin(18));
  EXPECT_EQ(sd.getCurrentStatus(), STATUS_INITIALIZED);

  EXPECT_CALL(net, isReady()).WillRepeatedly(Return(true));
  EXPECT_CALL(net, setup()).Times(1);
  EXPECT_CALL(net, iterate()).Times(AtLeast(1));
  EXPECT_CALL(srpc, srpc_iterate(_)).WillRepeatedly(Return(SUPLA_RESULT_TRUE));
  EXPECT_CALL(*client, stop()).Times(0);

  EXPECT_CALL(*client, connected()).WillOnce(Return(false))
    .WillRepeatedly(Return(true));
  EXPECT_CALL(*client, connectImp(_, 2016)).WillRepeatedly(Return(1));
  EXPECT_CALL(srpc, srpc_ds_async_registerdevice_e(_, _)).Times(1);

  EXPECT_CALL(el1, iterateAlways()).Times(AtLeast(1));
  EXPECT_CALL(el2, iterateAlways()).Times(AtLeast(1));
  EXPECT_CALL(el1, iterateConnected(_)).Times(AtLeast(1))
        .WillOnce(Return(false))
        .WillRepeatedly(Return(true));
  EXPECT_CALL(el2, iterateConnected(_)).Times(AtLeast(1));

  EXPECT_CALL(board, deviceSoftwareReset()).Times(0);
//  EXPECT_CALL(el1, onSaveState()).Times(AtLeast(1));
//  EXPECT_CALL(el2, onSaveState()).Times(AtLeast(1));

  EXPECT_EQ(sd.getCurrentStatus(), STATUS_INITIALIZED);
  for (int i = 0; i < 5; i++) {
    sd.iterate();
    time.advance(100);
  }
  EXPECT_EQ(sd.getCurrentStatus(), STATUS_REGISTER_IN_PROGRESS);

  TSD_SuplaRegisterDeviceResult register_device_result{};
  register_device_result.result_code = SUPLA_RESULTCODE_TRUE;
  register_device_result.activity_timeout = 45;
  register_device_result.version = 20;
  register_device_result.version_min = 1;

  auto srpcLayer = sd.getSrpcLayer();
  srpcLayer->onRegisterResult(&register_device_result);
  time.advance(100);

  EXPECT_EQ(sd.getCurrentStatus(), STATUS_REGISTERED_AND_READY);

  for (int i = 0; i < 10; i++) {
    sd.iterate();
    time.advance(100);
  }

  EXPECT_EQ(sd.getCurrentStatus(), STATUS_REGISTERED_AND_READY);
  EXPECT_STREQ(client->getRootCACert(), "custom ca");
}

TEST_F(SuplaDeviceTestsFullStartupManual,
    WithConfigSslEnabledCustomCASelectedButMissing) {
  ConfigMock cfg;
  const char myCA1[] = "test CA1";
  const char myCA2[] = "test CA2";
  sd.setSuplaCACert(myCA1);
  sd.setSupla3rdPartyCACert(myCA2);

  EXPECT_NE(Supla::Storage::ConfigInstance(), nullptr);
  int dummy = 0;
  sd.setActivityTimeout(45);


  EXPECT_CALL(cfg, init()).WillOnce(Return(true));
  EXPECT_CALL(cfg, isConfigModeSupported()).WillRepeatedly(Return(true));
  EXPECT_CALL(cfg, getDeviceName(_)).WillRepeatedly(Return(false));
  EXPECT_CALL(cfg, getGUID(_)).WillRepeatedly([] (char *guid) {
      char GUID[SUPLA_GUID_SIZE] = {1};
      memcpy(guid, GUID, SUPLA_GUID_SIZE);
      return true;
    });
  EXPECT_CALL(cfg, getAuthKey(_)).WillRepeatedly([] (char *auth) {
      char AUTHKEY[SUPLA_AUTHKEY_SIZE] = {2};
      memcpy(auth, AUTHKEY, SUPLA_AUTHKEY_SIZE);
      return true;
    });
  EXPECT_CALL(cfg, getDeviceMode())
    .WillRepeatedly(Return(Supla::DEVICE_MODE_NOT_SET));
  EXPECT_CALL(cfg, getSuplaServer(_)).WillRepeatedly([] (char *server) {
      char temp[] = "mega.supla.org";
      memcpy(server, temp, strlen(temp));
      return true;
    });
  EXPECT_CALL(cfg, getEmail(_)).WillRepeatedly([] (char *mail) {
      char temp[] = "ceo@supla.org";
      memcpy(mail, temp, strlen(temp));
      return true;
    });
  // ssl enabled in config is realized by setting port to -1 (auto/default)
  EXPECT_CALL(cfg, getSuplaServerPort()).WillRepeatedly(Return(-1));
  EXPECT_CALL(cfg, saveIfNeeded()).Times(AtLeast(1));
  EXPECT_CALL(cfg, getUInt8(_, _)).WillRepeatedly([] (const char *key,
        uint8_t *buf) {
      if (strcmp(key, "security_level") == 0) {
        *buf = 1;
        return true;
      }
      EXPECT_TRUE(false);
      return false;
    });
  EXPECT_CALL(cfg, getCustomCASize())
    .WillRepeatedly(Return(0));
  EXPECT_CALL(cfg, getCustomCA(_, _)).WillRepeatedly(Return(false));

  EXPECT_CALL(cfg, isSuplaCommProtocolEnabled()).WillRepeatedly(Return(true));

  EXPECT_CALL(el1, onLoadConfig()).Times(1);
  EXPECT_CALL(el2, onLoadConfig()).Times(1);
  EXPECT_CALL(el1, onInit()).Times(1);
  EXPECT_CALL(el2, onInit()).Times(1);
  EXPECT_CALL(el1, onRegistered(_));
  EXPECT_CALL(el2, onRegistered(_));

  EXPECT_CALL(timer, initTimers());
  EXPECT_CALL(srpc, srpc_params_init(_));
  EXPECT_CALL(srpc, srpc_init(_)).WillOnce(Return(&dummy));
  EXPECT_CALL(srpc, srpc_set_proto_version(&dummy, 18));

  EXPECT_TRUE(sd.begin(18));
  EXPECT_EQ(sd.getCurrentStatus(), STATUS_INITIALIZED);

  EXPECT_CALL(net, isReady()).WillRepeatedly(Return(true));
  EXPECT_CALL(net, setup()).Times(1);
  EXPECT_CALL(net, iterate()).Times(AtLeast(1));
  EXPECT_CALL(srpc, srpc_iterate(_)).WillRepeatedly(Return(SUPLA_RESULT_TRUE));
  EXPECT_CALL(*client, stop()).Times(0);

  EXPECT_CALL(*client, connected()).WillOnce(Return(false))
    .WillRepeatedly(Return(true));
  EXPECT_CALL(*client, connectImp(_, 2016)).WillRepeatedly(Return(1));
  EXPECT_CALL(srpc, srpc_ds_async_registerdevice_e(_, _)).Times(1);

  EXPECT_CALL(el1, iterateAlways()).Times(AtLeast(1));
  EXPECT_CALL(el2, iterateAlways()).Times(AtLeast(1));
  EXPECT_CALL(el1, iterateConnected(_)).Times(AtLeast(1))
        .WillOnce(Return(false))
        .WillRepeatedly(Return(true));
  EXPECT_CALL(el2, iterateConnected(_)).Times(AtLeast(1));

  EXPECT_CALL(board, deviceSoftwareReset()).Times(0);
//  EXPECT_CALL(el1, onSaveState()).Times(AtLeast(1));
//  EXPECT_CALL(el2, onSaveState()).Times(AtLeast(1));

  EXPECT_EQ(sd.getCurrentStatus(), STATUS_INITIALIZED);
  for (int i = 0; i < 5; i++) {
    sd.iterate();
    time.advance(100);
  }
  EXPECT_EQ(sd.getCurrentStatus(), STATUS_REGISTER_IN_PROGRESS);

  TSD_SuplaRegisterDeviceResult register_device_result{};
  register_device_result.result_code = SUPLA_RESULTCODE_TRUE;
  register_device_result.activity_timeout = 45;
  register_device_result.version = 20;
  register_device_result.version_min = 1;

  auto srpcLayer = sd.getSrpcLayer();
  srpcLayer->onRegisterResult(&register_device_result);
  time.advance(100);

  EXPECT_EQ(sd.getCurrentStatus(), STATUS_REGISTERED_AND_READY);

  for (int i = 0; i < 10; i++) {
    sd.iterate();
    time.advance(100);
  }

  EXPECT_EQ(sd.getCurrentStatus(), STATUS_REGISTERED_AND_READY);
  // "SUPLA" string is set as a dummy CA value (invalid), so connection in
  // production code will fail
  EXPECT_STREQ(client->getRootCACert(), "SUPLA");
}

TEST_F(SuplaDeviceTestsFullStartupManual,
    WithConfigSslEnabledInsecure) {
  ConfigMock cfg;
  const char myCA1[] = "test CA1";
  const char myCA2[] = "test CA2";
  sd.setSuplaCACert(myCA1);
  sd.setSupla3rdPartyCACert(myCA2);

  EXPECT_NE(Supla::Storage::ConfigInstance(), nullptr);
  int dummy = 0;
  sd.setActivityTimeout(45);


  EXPECT_CALL(cfg, init()).WillOnce(Return(true));
  EXPECT_CALL(cfg, isConfigModeSupported()).WillRepeatedly(Return(true));
  EXPECT_CALL(cfg, getDeviceName(_)).WillRepeatedly(Return(false));
  EXPECT_CALL(cfg, getGUID(_)).WillRepeatedly([] (char *guid) {
      char GUID[SUPLA_GUID_SIZE] = {1};
      memcpy(guid, GUID, SUPLA_GUID_SIZE);
      return true;
    });
  EXPECT_CALL(cfg, getAuthKey(_)).WillRepeatedly([] (char *auth) {
      char AUTHKEY[SUPLA_AUTHKEY_SIZE] = {2};
      memcpy(auth, AUTHKEY, SUPLA_AUTHKEY_SIZE);
      return true;
    });
  EXPECT_CALL(cfg, getDeviceMode())
    .WillRepeatedly(Return(Supla::DEVICE_MODE_NOT_SET));
  EXPECT_CALL(cfg, getSuplaServer(_)).WillRepeatedly([] (char *server) {
      char temp[] = "mega.supla.org";
      memcpy(server, temp, strlen(temp));
      return true;
    });
  EXPECT_CALL(cfg, getEmail(_)).WillRepeatedly([] (char *mail) {
      char temp[] = "ceo@supla.org";
      memcpy(mail, temp, strlen(temp));
      return true;
    });
  // ssl enabled in config is realized by setting port to -1 (auto/default)
  EXPECT_CALL(cfg, getSuplaServerPort()).WillRepeatedly(Return(-1));
  EXPECT_CALL(cfg, saveIfNeeded()).Times(AtLeast(1));
  EXPECT_CALL(cfg, getUInt8(_, _)).WillRepeatedly([] (const char *key,
        uint8_t *buf) {
      if (strcmp(key, "security_level") == 0) {
        *buf = 2;
        return true;
      }
      EXPECT_TRUE(false);
      return false;
    });

  EXPECT_CALL(cfg, isSuplaCommProtocolEnabled()).WillRepeatedly(Return(true));

  EXPECT_CALL(el1, onLoadConfig()).Times(1);
  EXPECT_CALL(el2, onLoadConfig()).Times(1);
  EXPECT_CALL(el1, onInit()).Times(1);
  EXPECT_CALL(el2, onInit()).Times(1);
  EXPECT_CALL(el1, onRegistered(_));
  EXPECT_CALL(el2, onRegistered(_));

  EXPECT_CALL(timer, initTimers());
  EXPECT_CALL(srpc, srpc_params_init(_));
  EXPECT_CALL(srpc, srpc_init(_)).WillOnce(Return(&dummy));
  EXPECT_CALL(srpc, srpc_set_proto_version(&dummy, 18));

  EXPECT_TRUE(sd.begin(18));
  EXPECT_EQ(sd.getCurrentStatus(), STATUS_INITIALIZED);

  EXPECT_CALL(net, isReady()).WillRepeatedly(Return(true));
  EXPECT_CALL(net, setup()).Times(1);
  EXPECT_CALL(net, iterate()).Times(AtLeast(1));
  EXPECT_CALL(srpc, srpc_iterate(_)).WillRepeatedly(Return(SUPLA_RESULT_TRUE));
  EXPECT_CALL(*client, stop()).Times(0);

  EXPECT_CALL(*client, connected()).WillOnce(Return(false))
    .WillRepeatedly(Return(true));
  EXPECT_CALL(*client, connectImp(_, 2016)).WillRepeatedly(Return(1));
  EXPECT_CALL(srpc, srpc_ds_async_registerdevice_e(_, _)).Times(1);

  EXPECT_CALL(el1, iterateAlways()).Times(AtLeast(1));
  EXPECT_CALL(el2, iterateAlways()).Times(AtLeast(1));
  EXPECT_CALL(el1, iterateConnected(_)).Times(AtLeast(1))
        .WillOnce(Return(false))
        .WillRepeatedly(Return(true));
  EXPECT_CALL(el2, iterateConnected(_)).Times(AtLeast(1));

  EXPECT_CALL(board, deviceSoftwareReset()).Times(0);
//  EXPECT_CALL(el1, onSaveState()).Times(AtLeast(1));
//  EXPECT_CALL(el2, onSaveState()).Times(AtLeast(1));

  EXPECT_EQ(sd.getCurrentStatus(), STATUS_INITIALIZED);
  for (int i = 0; i < 5; i++) {
    sd.iterate();
    time.advance(100);
  }
  EXPECT_EQ(sd.getCurrentStatus(), STATUS_REGISTER_IN_PROGRESS);

  TSD_SuplaRegisterDeviceResult register_device_result{};
  register_device_result.result_code = SUPLA_RESULTCODE_TRUE;
  register_device_result.activity_timeout = 45;
  register_device_result.version = 20;
  register_device_result.version_min = 1;

  auto srpcLayer = sd.getSrpcLayer();
  srpcLayer->onRegisterResult(&register_device_result);
  time.advance(100);

  EXPECT_EQ(sd.getCurrentStatus(), STATUS_REGISTERED_AND_READY);

  for (int i = 0; i < 10; i++) {
    sd.iterate();
    time.advance(100);
  }

  EXPECT_EQ(sd.getCurrentStatus(), STATUS_REGISTERED_AND_READY);
  EXPECT_EQ(client->getRootCACert(), nullptr);
}

TEST_F(SuplaDeviceElementWithSecondaryChannel, SuccessfulStartup) {
  bool isConnected = false;
  EXPECT_CALL(net, isReady()).WillRepeatedly(Return(true));
  EXPECT_CALL(*client, connected()).WillRepeatedly(ReturnPointee(&isConnected));
  EXPECT_CALL(*client, connectImp(_, _)).WillRepeatedly(DoAll(Assign(&isConnected, true), Return(1)));

  EXPECT_CALL(net, setup()).Times(1);
  EXPECT_CALL(net, iterate()).Times(AtLeast(1));
  EXPECT_CALL(srpc, srpc_iterate(_)).WillRepeatedly(Return(SUPLA_RESULT_TRUE));

  EXPECT_CALL(el1, iterateAlways()).Times(35);
  EXPECT_CALL(el2, iterateAlways()).Times(35);

  EXPECT_CALL(el1, onRegistered(_));
  EXPECT_CALL(el2, onRegistered(_));

  EXPECT_CALL(srpc, srpc_ds_async_registerdevice_e(_, _)).Times(1);
  EXPECT_CALL(srpc, srpc_dcs_async_set_activity_timeout(_, _)).Times(1);
  EXPECT_CALL(srpc, srpc_dcs_async_ping_server(_)).Times(2);

//  EXPECT_CALL(net, ping(_)).WillRepeatedly(Return(true));
  EXPECT_CALL(el1, iterateConnected(_)).Times(30).WillRepeatedly(Return(true));
  EXPECT_CALL(el2, iterateConnected(_)).Times(30).WillRepeatedly(Return(true));

  EXPECT_EQ(sd.getCurrentStatus(), STATUS_INITIALIZED);
  for (int i = 0; i < 5; i++) {
    sd.iterate();
    time.advance(1000);
  }
  EXPECT_EQ(sd.getCurrentStatus(), STATUS_REGISTER_IN_PROGRESS);

  TSD_SuplaRegisterDeviceResult register_device_result{};
  register_device_result.result_code = SUPLA_RESULTCODE_TRUE;
  register_device_result.activity_timeout = 45;
  register_device_result.version = 16;
  register_device_result.version_min = 1;

  auto srpcLayer = sd.getSrpcLayer();
  srpcLayer->onRegisterResult(&register_device_result);
  time.advance(100);

  EXPECT_EQ(sd.getCurrentStatus(), STATUS_REGISTERED_AND_READY);

  for (int i = 0; i < 30; i++) {
    sd.iterate();
    time.advance(1000);
  }

  EXPECT_EQ(sd.getCurrentStatus(), STATUS_REGISTERED_AND_READY);
}
