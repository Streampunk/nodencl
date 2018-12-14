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

const addon = require('bindings')('nodencl');

const SegfaultHandler = require('segfault-handler');
SegfaultHandler.registerHandler('crash.log'); // With no argument, SegfaultHandler will generate a generic log file name

function getPlatformInfo() {
  return addon.getPlatformInfo();
}

async function createContext(params) {
  return (0 === Object.keys(params).length) ? await addon.createContext() :
    await addon.createContext({
      platformIndex: params.platformIndex, 
      deviceIndex: params.deviceIndex
    });
}

function addReference(buffer, buffers) {
  // console.log(`addRef ${buffer.index}: ${buffer.owner} ${buffer.length} bytes - refs ${buffer.refs}, ${buffer.reserved?'reserved':'free'}`);
  if (!buffers.find(el => el.index === buffer.index))
    console.error(`addReference on freed buffer ${buffer.index}: ${buffer.owner} ${buffer.length} bytes`);
  if (!buffer.reserved)
    console.warn(`addReference on unreserved buffer ${buffer.index}: ${buffer.owner} ${buffer.length} bytes`);
  buffer.refs++;
}

function releaseReference(buffer) {
  // console.log(`release ${buffer.index}: ${buffer.owner} ${buffer.length} bytes - refs ${buffer.refs}, ${buffer.reserved?'reserved':'free'}`);
  if (!buffer.reserved)
    console.warn(`releaseReference on unreserved buffer ${buffer.index}: ${buffer.owner} ${buffer.length} bytes`);
  if (buffer.refs > 0) buffer.refs--;
  if (0 === buffer.refs)
    buffer.reserved = false;
}

function clContext(params, logger) {
  this.logger = logger || { log: console.log, warn: console.warn, error: console.error };
  this.buffers = [];
  this.bufIndex = 0;

  this.getPlatformInfo = async () => {
    const c = await this.context;
    return addon.getPlatformInfo()[c.platformIndex];
  };

  this.logBuffers = () => this.buffers.forEach(el => 
    this.logger.log(`${el.index}: ${el.owner} ${el.length} bytes ${el.reserved?'reserved':'available'}`));

  this.context = createContext(params);
}

clContext.prototype.checkAlloc = async function(cb) {
  let result;
  try {
    result = await cb();
  } catch (err) {
    if (-4 == err.code) { // memory allocation failure
      this.logger.warn('Failed to allocate OpenCL memory - freeing unreserved allocations');
      this.buffers = this.buffers.filter(el => {
        if (!el.reserved) el.freeAllocation();
        return el.reserved === true;
      });
      result = await cb();
    } else
      throw err;
  }
  return result;
};

clContext.prototype.createBuffer = async function(numBytes, bufDir, bufType, owner) {
  const buf = this.buffers.find(el => 
    !el.reserved && (el.length === numBytes) && (el.bufDir === bufDir) && (el.bufType === bufType));
  if (buf) {
    this.logger.log(`reuse ${owner}: ${buf.owner} ${numBytes} bytes`);
    buf.reserved = true;
    buf.owner = owner;
    buf.refs = 0;
    return buf;
  } else return this.context
    .then(c => this.checkAlloc(() => c.createBuffer(numBytes, bufDir, bufType)))
    .then(buf => {
      buf.reserved = true;
      buf.owner = owner;
      buf.index = this.bufIndex++;
      buf.bufDir = bufDir;
      buf.bufType = bufType;
      buf.refs = 0;
      buf.addRef = () => addReference(buf, this.buffers);
      buf.release = () => releaseReference(buf);
      if (owner) this.buffers.push(buf);
      return buf;
    });
};

clContext.prototype.releaseBuffers = function(owner) {
  this.buffers = this.buffers.filter(el => {
    if (el.owner === owner) el.freeAllocation();
    return el.owner !== owner; 
  });
};

clContext.prototype.createProgram = async function(kernel, options) {
  return this.context
    .then(c => c.createProgram(kernel, options));
};

clContext.prototype.runProgram = async function(program, params) {
  return await this.checkAlloc(() => program.run(params));
};

clContext.prototype.close = function(done) {
  const i = setInterval(() => {
    if (0 === this.buffers.length) {
      this.logger.log('All OpenCL allocations have been released');
      clearInterval(this.bufLog);
      clearInterval(i);
      clearInterval(t);
      this.context = null;
      if (done) done();
    }
  }, 20);
  const t = setTimeout(() => {
    clearInterval(this.bufLog);
    clearInterval(i);
    this.logger.warn('Timed out waiting for release of OpenCL allocations');
    this.buffers = this.buffers.map(el => el.freeAllocation());
    this.buffers.length = 0;
    this.context = null;
    if (done) done();
  }, 1000);
};

module.exports = {
  getPlatformInfo,
  clContext 
};
