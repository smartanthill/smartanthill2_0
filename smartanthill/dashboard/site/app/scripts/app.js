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

angular.module('siteApp', [
  'ngAnimate',
  'ngResource',
  'ngRoute',
  'ngSanitize',
  'ngTouch',
  'ui.bootstrap',
  'ui.select',
  'toaster',
  'ngTable'
])

.constant('siteConfig', {
  apiURL: (parseInt(location.port) === 9000 ? '//localhost:8138' : '') + '/api/'
})

.config(function($routeProvider) {
  $routeProvider
    .when('/', {
      templateUrl: 'views/dashboard.html',
      controller: 'DashboardCtrl'
    })
    .when('/devices/add', {
      templateUrl: 'views/device_addoredit.html',
      controller: 'DeviceAddOrEditCtrl'
    })
    .when('/devices/:deviceId/edit', {
      templateUrl: 'views/device_addoredit.html',
      controller: 'DeviceAddOrEditCtrl'
    })
    .when('/devices/:deviceId', {
      templateUrl: 'views/device_info.html',
      controller: 'DeviceInfoCtrl'
    })
    .when('/devices', {
      templateUrl: 'views/devices.html',
      controller: 'DevicesCtrl'
    })
    .when('/network', {
      templateUrl: 'views/network.html',
      controller: 'NetworkCtrl'
    })
    .when('/console', {
      templateUrl: 'views/console.html',
      controller: 'ConsoleCtrl'
    })
    .when('/settings', {
      templateUrl: 'views/settings.html',
      controller: 'SettingsController',
      controllerAs: 'vm',
    })
    .otherwise({
      redirectTo: '/'
    });
})

.factory('siteStorage', function($http, $resource, $cacheFactory, siteConfig) {
  return {
    operations: $resource(siteConfig.apiURL + 'operations'),
    boards: $resource(siteConfig.apiURL + 'boards/:boardId', {
      boardId: '@id'
    }),
    devices: $resource(siteConfig.apiURL + 'devices/:deviceId', {
      deviceId: '@id'
    }),
    serialports: $resource(siteConfig.apiURL + 'serialports')
  };
})

.factory('notifyUser', function($log, toaster) {
  return function(type, message) {
    switch (type) {
      case 'success':
        $log.info(message);
        toaster.pop('success', message);
        break;

      case 'warning':
        $log.warn(message);
        toaster.pop('warning', message);
        break;

      default:
        $log.error(message);
        toaster.pop('error', message);
        break;
    }
  };
})

.controller('SiteController', function($scope, $location) {

  $scope.isRouteActive = function(route) {
    return $location.path().lastIndexOf(route, 0) === 0;
  };

  $scope.$watch(function() {
    return $location.path();
  }, function(path) {
    if (path === '/') {
      path = '/dashboard';
    }
    path = path.substring(1);
    $scope.currentPage = path.substring(0, 1).toUpperCase() + path.substring(1);
  });

});
