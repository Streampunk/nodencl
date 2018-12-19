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

const pi = 0;
const di = 0;
const properties = { platformIndex: pi, deviceIndex: di };
function createContext(description, cb) {
  tape(description, async t => {
    const clContext = new addon.clContext(properties);
    try {
      await cb(t, clContext);
      clContext.close(t.end);
    } catch (err) {
      t.fail(err);
      t.end();
    }
  });
}

const numBytes = 65536;
const bufDirs = [ 'readonly', 'writeonly', 'readwrite' ];
const svmTypes = [ 'none', 'coarse', 'fine' ];
for (let d=0; d<bufDirs.length; ++d) {
  for (let t=0; t<svmTypes.length; ++t) {
    const dir = bufDirs[d];
    const svm = svmTypes[t];
    createContext(`Create OpenCL buffer direction ${dir} SVM type ${svm}`, async (t, clContext) => {
      const testBuffer = await clContext.createBuffer(numBytes, dir, svm);
      t.ok(Buffer.isBuffer(testBuffer), 'buffer created ok');
      t.equal(testBuffer.numBytes, numBytes, 'buffer has correct size');
      t.ok(testBuffer.hasOwnProperty('hostAccess'), 'has hostAccess function');
    });
  }
}

createContext('Create buffer with negative size', async (t, clContext) => {
  try {
    await clContext.createBuffer(-numBytes, 'readwrite');
    t.fail('negative buffer size parameter should give error');
  } catch (err) {
    t.pass(`negative buffer size parameter produces ${err}`);
  }
});

createContext('Create buffer with no direction parameter', async (t, clContext) => {
  try {
    await clContext.createBuffer(numBytes);
    t.fail('no buffer direction parameter should give error');
  } catch (err) {
    t.pass(`no buffer direction parameter produces ${err}`);
  }
});

createContext('Create buffer with incorrect direction parameter', async (t, clContext) => {
  try {
    await clContext.createBuffer(numBytes, 'writeread');
    t.fail('incorrect buffer direction parameter should give error');
  } catch (err) {
    t.pass(`incorrect buffer direction parameter produces ${err}`);
  }
});

createContext('Create buffer with incorrect type parameter', async (t, clContext) => {
  try {
    await clContext.createBuffer(numBytes, 'readwrite', 'rough');
    t.fail('incorrect buffer type parameter should give error');
  } catch (err) {
    t.pass(`incorrect buffer type parameter produces ${err}`);
  }
});

createContext('Create buffer and request host access', async (t, clContext) => {
  const testBuffer = await clContext.createBuffer(numBytes, 'readwrite', 'none');
  await testBuffer.hostAccess('readwrite');
  t.pass('host access requested OK');
});

createContext('Create buffer and request host access with source copy', async (t, clContext) => {
  const srcBuf = Buffer.alloc(numBytes, 0xe5);
  const testBuffer = await clContext.createBuffer(numBytes, 'readwrite', 'none');
  await testBuffer.hostAccess('readwrite', srcBuf);
  t.pass('host access requested OK');
  t.deepEqual(testBuffer, srcBuf, 'buffer contains expected data');
});

createContext('Create buffer and request host access with incorrect access parameter', async (t, clContext) => {
  const testBuffer = await clContext.createBuffer(numBytes, 'readwrite', 'none');
  try {
    await testBuffer.hostAccess('writeread');
    t.fail('incorrect host access parameter should give error');
  } catch (err) {
    t.pass(`incorrect host access parameter produces ${err}`);
  }
});
