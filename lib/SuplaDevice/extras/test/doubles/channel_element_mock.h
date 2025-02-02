/*
 * Copyright (C) AC SOFTWARE SP. Z O.O
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef EXTRAS_TEST_DOUBLES_CHANNEL_ELEMENT_MOCK_H_
#define EXTRAS_TEST_DOUBLES_CHANNEL_ELEMENT_MOCK_H_

#include <gmock/gmock.h>
#include <supla/channel_element.h>

class ChannelElementMock : public Supla::ChannelElement {
  public:
  MOCK_METHOD(void, onInit, (), (override));
  MOCK_METHOD(void, onLoadState, (), (override));
  MOCK_METHOD(void, onSaveState, (), (override));
  MOCK_METHOD(void, onRegistered, (Supla::Protocol::SuplaSrpc *), (override));
  MOCK_METHOD(void, iterateAlways, (), (override));
  MOCK_METHOD(bool, iterateConnected, (void *), (override));
  MOCK_METHOD(void, onTimer, (), (override));
  MOCK_METHOD(void, onFastTimer, (), (override));
  MOCK_METHOD(int, handleNewValueFromServer, (TSD_SuplaChannelNewValue *), (override));
  MOCK_METHOD(void, handleGetChannelState, (TDSC_ChannelState *), (override));
  MOCK_METHOD(int, handleCalcfgFromServer, (TSD_DeviceCalCfgRequest *), (override));
};

#endif  // EXTRAS_TEST_DOUBLES_CHANNEL_ELEMENT_MOCK_H_
