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
    dataService, device, operationsList, serialPortsList) {

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
    vm.deferred.promise.then(function() {
      vm.btnDisabled.start = true;
      vm.progressbar.value = 20;
      vm.progressbar.info = 'Building...';

      return dataService.buildDeviceFirmware(device.id).$promise;
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
        return dataService.deviceUploadFirmware(device.id, data).$promise;
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

    vm.start = function() {
      vm.deferred.resolve();
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
