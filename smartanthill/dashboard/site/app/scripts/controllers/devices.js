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
        data.data + ')'));
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
