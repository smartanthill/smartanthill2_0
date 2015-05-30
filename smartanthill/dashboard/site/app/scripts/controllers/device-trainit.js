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
    .controller('DeviceTrainItController', DeviceTrainItController);

  function DeviceTrainItController($q, $resource, $modalInstance,
    siteConfig, dataService, device, operationsList, serialPortsList) {

    var vm = this;

    vm.serialports = serialPortsList;
    vm.device = device;
    vm.selectedSerialPort = {};
    vm.progressbar = {
      value: 0,
      info: ''

    };
    vm.btnDisabled = {
      start: false,
      cancel: false
    };

    // Deferred chain
    vm.deferCancalled = false;
    vm.deferred = $q.defer();
    vm.deferred.promise.then(function(ccopts) {
      vm.btnDisabled.start = true;
      vm.progressbar.value = 20;
      vm.progressbar.info = 'Building...';

      return $resource('http://localhost:8130/').save(ccopts).$promise;
    }, function(failure) {
      return $q.reject(failure);
    })

    // building promise
    .then(function(result) {
        if (vm.deferCancalled) {
          return $q.reject();
        }

        vm.btnDisabled.cancel = true;
        vm.progressbar.value = 60;
        vm.progressbar.info = 'Uploading...';

        var data = result;
        data.uploadport = vm.selectedSerialPort.selected.port;
        return $resource(siteConfig.apiURL + 'devices/:deviceId/uploadfw', {
          deviceId: device.id
        }).save(data).$promise;
      },
      function(failure) {
        var errMsg = '';
        if (!failure || angular.isString(failure)) {
          errMsg = failure;
        } else {
          errMsg = 'An unexpected error occurred when building firmware.';
          if (angular.isObject(failure) && failure.data) {
            errMsg += ' ' + failure.data;
          }
        }
        return $q.reject(errMsg);
      })

    // uploading promise
    .then(function(result) {
        if (vm.deferCancalled) {
          return $q.reject();
        }
        vm.progressbar.value = 100;
        vm.progressbar.info = 'Completed!';
        return result.result;
      },
      function(failure) {
        var errMsg = '';
        if (!failure || angular.isString(failure)) {
          errMsg = failure;
        } else {
          errMsg =
            'An unexpected error occurred when uploading firmware.';
          if (angular.isObject(failure) && failure.data) {
            errMsg += ' ' + failure.data;
          }
        }
        return $q.reject(errMsg);
      })

    .then(function(result) {
        console.log(result);
        $modalInstance.close(
          'The device has been successfully Train It!-ed. ' +
          result);
      },
      function(failure) {
        if (!failure && vm.btnDisabled.start) {
          failure = 'The Train It! operation has been cancelled!';
        }
        $modalInstance.dismiss(failure);
      });

    /**
     * Preparing defines for Cloud Compiler
     */
    var ccDefines = {
      DEVICE_ID: device.id
    };

    // process operations
    angular.forEach(operationsList, function(item) {
      if (device.operationIds.indexOf(item.id) === -1) {
        return;
      }
      ccDefines[item.name] = item.id;
    });

    // process network data
    var _routerURI = device.network.router;
    switch (_routerURI.substring(0, _routerURI.indexOf(':'))) {
      case 'serial':
        ccDefines.ROUTER_SERIAL = null;
        var match = new RegExp('baudrate=(\\d+)', 'i').exec(_routerURI);
        ccDefines.ROUTER_SERIAL_BAUDRATE = parseInt(match ? match[1] : 9600);
        break;
    }

    // console.log(ccDefines);

    vm.start = function() {
      vm.deferred.resolve({
        'pioenv': device.boardId,
        'defines': ccDefines
      });
    };

    vm.cancel = function() {
      vm.deferred.reject();
      vm.deferCancalled = true;
    };

    vm.finish = function() {
      $modalInstance.close(
        'The device has been successfully Train It!-ed.');
    };
  }

})();
