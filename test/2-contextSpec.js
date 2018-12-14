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

const platformInfo = addon.getPlatformInfo();

function createContext(description, properties, cb) {
  tape(description, async t => {
    const clContext = new addon.clContext(properties);
    try {
      cb(null, t, await clContext.context);
      clContext.close(t.end);
    } catch (error) {
      cb(error, t);
      t.end();
    }
  });
}

platformInfo.forEach((platform, pi) => {
  platform.devices.forEach((device, di) => {
    createContext(`Create OpenCL context - platform ${pi}, device ${di}`, { platformIndex: pi, deviceIndex: di }, (err, t, context) => {
      if (err) {
        t.fail(err);
      } else {
        t.equal(context.platformIndex, pi, 'context has the expected platform index');
        t.equal(context.deviceIndex, di, 'context has the expected device index');
        t.equal(device.platformIndex, pi, 'device has the expected platform index');
        t.equal(device.deviceIndex, di, 'device has the expected device index');
        t.ok(context.hasOwnProperty('createProgram'), 'has createProgram function');
        t.ok(context.hasOwnProperty('createBuffer'), 'has createBuffer function');
      }
    });
  });
});

createContext('Create OpenCL context with no parameters', {}, (err, t, context) => {
  if (err)
    t.pass(`no parameters produces ${err}`);
  else {
    t.ok(context.hasOwnProperty('createProgram'), 'has createProgram function');
    t.ok(context.hasOwnProperty('createBuffer'), 'has createBuffer function');
  }
});

let pi = platformInfo.length;
let di = 0;
createContext(`Create OpenCL context - platform ${pi}, device ${di}`, { platformIndex: pi, deviceIndex: di }, (err, t) => {
  if (err)
    t.pass(`platform index out of range produces ${err}`);
  else
    t.fail('platform index out of range should produce an error');
});

pi = 0;
di = platformInfo[pi].devices.length;
createContext(`Create OpenCL context - platform ${pi}, device ${di}`, { platformIndex: pi, deviceIndex: di }, (err, t) => {
  if (err)
    t.pass(`device index out of range produces ${err}`);
  else
    t.fail('device index out of range should produce an error');
});

pi = 0;
di = 0;
createContext('Create OpenCL context with no platform index', { deviceIndex: di }, (err, t) => {
  if (err)
    t.pass(`negative platform index produces ${err}`);
  else
    t.fail('negative platform index should produce an error');
});

createContext('Create OpenCL context with no device index', { platformIndex: pi }, (err, t) => {
  if (err)
    t.pass(`no device index produces ${err}`);
  else
    t.fail('no device index should produce an error');
});

pi = -1;
di = 0;
createContext(`Create OpenCL context - platform ${pi}, device ${di}`, { platformIndex: pi, deviceIndex: di }, (err, t) => {
  if (err)
    t.pass(`negative platform index produces ${err}`);
  else
    t.fail('negative platform index should produce an error');
});

pi = 0;
di = -1;
createContext(`Create OpenCL context - platform ${pi}, device ${di}`, { platformIndex: pi, deviceIndex: di }, (err, t) => {
  if (err)
    t.pass(`negative device index produces ${err}`);
  else
    t.fail('negative device index should produce an error');
});
