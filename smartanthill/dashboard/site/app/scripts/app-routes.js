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
    .config(routeConfig);

  function routeConfig($locationProvider, $routeProvider) {
    $locationProvider.hashPrefix('!');

    $routeProvider
      .when('/', {
        templateUrl: 'views/dashboard.html',
        controller: 'DashboardController',
        controllerAs: 'vm'
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
        templateUrl: 'views/device-info.html',
        controller: 'DeviceInfoController',
        controllerAs: 'vm',
        resolve: {
          deviceResource: ['$route', 'dataService',
            function($route, dataService) {
              return dataService.devices.get({
                deviceId: $route.current.params.deviceId
              }).$promise;
            }
          ],
          operationsList: ['dataService',
            function(dataService) {
              return dataService.operations.query().$promise;
            }
          ]
        }
      })
      .when('/devices', {
        templateUrl: 'views/devices.html',
        controller: 'DevicesCtrl'
      })
      .when('/network', {
        templateUrl: 'views/network.html',
        controller: 'NetworkController',
        controllerAs: 'vm'
      })
      .when('/console', {
        templateUrl: 'views/console.html',
        controller: 'ConsoleController',
        controllerAs: 'vm'
      })
      .when('/settings', {
        templateUrl: 'views/settings.html',
        controller: 'SettingsController',
        controllerAs: 'vm',
        resolve: {
          Settings: ['dataService',
            function(dataService) {
              return dataService.settings.get().$promise;
            }
          ],
          LoggerLevels: function getValidLoggerLevels() {
            // TODO: get available levels via API
            return ['FATAL', 'ERROR', 'WARN', 'INFO', 'DEBUG'];
          }
        }
      })
      .otherwise({
        redirectTo: '/'
      });
  }

})();
