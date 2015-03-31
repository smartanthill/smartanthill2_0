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

'use strict';

angular.module('siteApp')

.controller('DevicesCtrl', function($scope, dataService) {
  $scope.devices = dataService.devices.query();
})

.controller('DeviceInfoCtrl', function($scope, $routeParams, $location,
  $modal, dataService, notifyUser) {
  $scope.operations = dataService.operations.query();
  $scope.device = dataService.devices.get({
      deviceId: $routeParams.deviceId
    },
    function(data) {
      $scope.board = dataService.boards.get({
        boardId: data.boardId
      });
    });

  $scope.deleteDevice = function() {
    if (!window.confirm('Are sure you want to delete this device?')) {
      return false;
    }

    var devId = $scope.device.id;
    $scope.device.$delete()

    .then(function() {
      notifyUser(
        'success', 'Device #' + devId + ' has been successfully deleted!');

      $location.path('/devices');

    }, function(data) {
      notifyUser(
        'error', ('An unexpected error occurred when deleting device (' +
          data.data + ')')
      );
    });
  };

  $scope.trainIt = function() {
    var modalInstance = $modal.open({
      templateUrl: 'views/device_trainit.html',
      controller: 'DeviceTrainItCtrl',
      backdrop: false,
      keyboard: false,
      resolve: {
        device: function() {
          return $scope.device;
        },
        operations: function() {
          return $scope.operations;
        }
      }
    });

    modalInstance.result.then(function(result) {
      notifyUser('success', result);
    }, function(failure) {
      if (failure) {
        notifyUser('error', failure);
      }
    });
  };

})

.controller('DeviceTrainItCtrl', function($q, $scope, $resource,
  $modalInstance, siteConfig, dataService, device, operations) {

  $scope.serialports = dataService.serialports.query();
  $scope.device = device;
  $scope.selectedSerialPort = {};
  $scope.progressbar = {
    value: 0,
    info: ''

  };
  $scope.btnDisabled = {
    start: false,
    cancel: false
  };

  // Deferred chain
  $scope.deferCancalled = false;
  $scope.deferred = $q.defer();
  $scope.deferred.promise.then(function(ccopts) {
    $scope.btnDisabled.start = true;
    $scope.progressbar.value = 20;
    $scope.progressbar.info = 'Building...';

    return $resource('http://localhost:8130/').save(ccopts).$promise;
  }, function(failure) {
    return $q.reject(failure);
  })

  // building promise
  .then(function(result) {
      if ($scope.deferCancalled) {
        return $q.reject();
      }

      $scope.btnDisabled.cancel = true;
      $scope.progressbar.value = 60;
      $scope.progressbar.info = 'Uploading...';

      var data = result;
      data.uploadport = $scope.selectedSerialPort.selected.port;
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
      if ($scope.deferCancalled) {
        return $q.reject();
      }
      $scope.progressbar.value = 100;
      $scope.progressbar.info = 'Completed!';
      return result.result;
    },
    function(failure) {
      var errMsg = '';
      if (!failure || angular.isString(failure)) {
        errMsg = failure;
      } else {
        errMsg = 'An unexpected error occurred when uploading firmware.';
        if (angular.isObject(failure) && failure.data) {
          errMsg += ' ' + failure.data;
        }
      }
      return $q.reject(errMsg);
    })

  .then(function(result) {
    console.log(result);
      $modalInstance.close('The device has been successfully Train It!-ed. ' +
                           result);
    },
    function(failure) {
      if (!failure && $scope.btnDisabled.start) {
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
  angular.forEach(operations, function(item) {
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

  $scope.start = function() {
    $scope.deferred.resolve({
      'pioenv': device.boardId,
      'defines': ccDefines
    });
  };

  $scope.cancel = function() {
    $scope.deferred.reject();
    $scope.deferCancalled = true;
  };

  $scope.finish = function() {
    $modalInstance.close('The device has been successfully Train It!-ed.');
  };

})

.controller('DeviceAddOrEditCtrl', function($q, $scope, $routeParams,
  $location, dataService, notifyUser) {
  $scope.selectBoard = {};
  $scope.editMode = false;
  $scope.prevState = {};
  $scope.operations = dataService.operations.query();
  /**
   * Handlers
   */
  $scope.$watch('selectBoard.selected', function(newValue) {
    if (!angular.isObject(newValue)) {
      return;
    }
    $scope.device.boardId = newValue.id;
  });

  $scope.boardGroupBy = function(item) {
    return item.name.substr(0, item.name.indexOf('('));
  };

  $scope.flipDeviceOperIDs = function() {
    var _prevOpIDs = $scope.device.operationIds;
    $scope.device.operationIds = {};
    angular.forEach(_prevOpIDs, function(id) {
      $scope.device.operationIds[id] = true;
    });
  };

  $scope.submitForm = function() {
    if ($scope.device.id !== $scope.prevState.id &&
      usedDevIds[$scope.device.id]) {
      window.alert('This Device ID is already used by another device.');
      return;
    }

    $scope.submitted = true;
    $scope.disableSubmit = true;

    // convert dictionary to array
    var _prevOpIDs = $scope.device.operationIds;
    $scope.device.operationIds = [];
    angular.forEach(_prevOpIDs, function(value, id) {
      if (value) {
        $scope.device.operationIds.push(parseInt(id));
      }
    });

    $scope.device.$save()

    .then(function() {
      notifyUser('success', 'Settings has been successfully ' + (
        $scope.editMode ? 'updated' : 'added'));
      $location.path('/devices/' + $scope.device.id);
    }, function(data) {
      notifyUser('error', ('An unexpected error occurred when ' + (
        $scope.editMode ? 'updating' : 'adding') + ' settings (' +
        data.data + ')')
      );
      $scope.flipDeviceOperIDs();
    })

    .finally(function() {
      $scope.disableSubmit = false;
    });
  };

  $scope.resetForm = function() {
    angular.copy($scope.prevState, $scope.device);
    $scope.$broadcast('form-reset');
    $scope.submitted = false;
  };

  /* End Handlers block */

  var usedDevIds = {};
  dataService.devices.query(function(data) {
    angular.forEach(data, function(item) {
      usedDevIds[item.id] = true;
    });
  });

  if ($routeParams.deviceId) {

    var deferred = $q.defer();
    deferred.promise.then(function() {
      $scope.device = dataService.devices.get({
        deviceId: $routeParams.deviceId
      });
      return $scope.device.$promise;
    })

    .then(function() {
      $scope.boards = dataService.boards.query();
      return $scope.boards.$promise;
    })

    .then(function() {
      angular.forEach($scope.boards, function(item) {
        if (item.id === $scope.device.boardId) {
          $scope.selectBoard.selected = item;
        }
      });

      $scope.flipDeviceOperIDs();

      $scope.prevState = angular.copy($scope.device);
      $scope.editMode = true;
    });
    deferred.resolve();

  } else {

    $scope.boards = dataService.boards.query();
    $scope.device = new dataService.devices();

    // default operations
    $scope.device.operationIds = {};
    $scope.device.operationIds[0] = true; // PING
    $scope.device.operationIds[0x89] = true; // LIST_OPERATIONS

    $scope.prevState = angular.copy($scope.device);

  }

});
