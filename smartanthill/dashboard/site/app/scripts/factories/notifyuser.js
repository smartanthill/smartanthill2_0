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
    .factory('notifyUser', notifyUser);

  function notifyUser($log, toaster) {
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
  }

})();
