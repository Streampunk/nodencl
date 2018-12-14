/* Copyright 2018 Streampunk Media Ltd.

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

const addon = require('../index.js');
const tape = require('tape');

tape('Get platform info', t => {
  const platformInfo = addon.getPlatformInfo();
  t.ok(Array.isArray(platformInfo), 'platform info is an array');
  platformInfo.forEach((p, pi) => {
    t.ok(p.hasOwnProperty('profile'), `platform ${pi} has a profile`);
    t.ok(p.hasOwnProperty('version'), `platform ${pi} has a version`);
    t.ok(p.hasOwnProperty('name'), `platform ${pi} has a name`);
    t.ok(p.hasOwnProperty('devices'), `platform ${pi} has devices`);
    p.devices.forEach((d, di) => {
      t.ok(d.hasOwnProperty('vendor'), `platform ${pi}, device ${di} has a vendor`);
      t.ok(d.hasOwnProperty('version'), `platform ${pi}, device ${di} has a version`);
      t.ok(d.hasOwnProperty('name'), `platform ${pi}, device ${di} has a name`);
      t.ok(d.hasOwnProperty('type'), `platform ${pi}, device ${di} has a type`);
    });
  });
  t.end();
});
