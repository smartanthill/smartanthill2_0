/**
  Copyright (c) 2015, OLogN Technologies AG. All rights reserved.

  Redistribution and use of this file in source (.rst) and compiled
  (.html, .pdf, etc.) forms, with or without modification, are permitted
  provided that the following conditions are met:
      * Redistributions in source form must retain the above copyright
        notice, this list of conditions and the following disclaimer.
      * Redistributions in compiled form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution.
      * Neither the name of the OLogN Technologies AG nor the names of its
        contributors may be used to endorse or promote products derived from
        this software without specific prior written permission.
  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED. IN NO EVENT SHALL OLogN Technologies AG BE LIABLE FOR ANY
  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
  OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
  DAMAGE
 */

'use strict';

angular.module('siteApp')

.controller('DevicesCtrl', function($scope, siteStorage) {
  $scope.devices = siteStorage.devices.query();
})

.controller('DeviceInfoCtrl', function($scope, $routeParams, $location,
  $modal, siteStorage, notifyUser) {
  $scope.operations = siteStorage.operations.query();
  $scope.device = siteStorage.devices.get({
      deviceId: $routeParams.deviceId
    },
    function(data) {
      $scope.board = siteStorage.boards.get({
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
  $modalInstance, siteConfig, siteStorage, device, operations) {

  $scope.serialports = siteStorage.serialports.query();
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
  $location, siteStorage, notifyUser) {
  $scope.selectBoard = {};
  $scope.editMode = false;
  $scope.prevState = {};
  $scope.operations = siteStorage.operations.query();
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
  siteStorage.devices.query(function(data) {
    angular.forEach(data, function(item) {
      usedDevIds[item.id] = true;
    });
  });

  if ($routeParams.deviceId) {

    var deferred = $q.defer();
    deferred.promise.then(function() {
      $scope.device = siteStorage.devices.get({
        deviceId: $routeParams.deviceId
      });
      return $scope.device.$promise;
    })

    .then(function() {
      $scope.boards = siteStorage.boards.query();
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

    $scope.boards = siteStorage.boards.query();
    $scope.device = new siteStorage.devices();

    // default operations
    $scope.device.operationIds = {};
    $scope.device.operationIds[0] = true; // PING
    $scope.device.operationIds[0x89] = true; // LIST_OPERATIONS

    $scope.prevState = angular.copy($scope.device);

  }

});
