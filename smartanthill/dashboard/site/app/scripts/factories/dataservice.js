/**
  Copyright (C) 2015 OLogN Technologies AG

  This source file is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License version 2
  as published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

(function() {
  'use strict';

  angular
    .module('siteApp')
    .factory('dataService', dataService);

  function dataService($resource, siteConfig) {
    return {
      boards: boards(),
      devices: devices(),
      serialports: serialports(),
      settings: settings(),
      operations: operations(),
      buildDeviceFirmware: buildDeviceFirmware,
      deviceUploadFirmware: deviceUploadFirmware
    };

    function operations() {
      return $resource(siteConfig.apiURL + 'operations');
    }

    function boards() {
      return $resource(siteConfig.apiURL + 'boards/:boardId', {
        boardId: '@id'
      });
    }

    function devices() {
      return $resource(siteConfig.apiURL + 'devices/:deviceId', {
        deviceId: '@id'
      });
    }

    function serialports() {
      return $resource(siteConfig.apiURL + 'serialports');
    }

    function settings() {
      return $resource(siteConfig.apiURL + 'settings');
    }

    function buildDeviceFirmware(deviceId) {
      return $resource(siteConfig.apiURL + 'devices/:id/buildfw', {
        id: deviceId
      }).get();
    }

    function deviceUploadFirmware(deviceId, data) {
      return $resource(siteConfig.apiURL + 'devices/:id/uploadfw', {
        id: deviceId
      }).save(data);
    }
  }

})();
