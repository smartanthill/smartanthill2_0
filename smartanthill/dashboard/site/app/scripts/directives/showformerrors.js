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
    .directive('showFormErrors', showFormErrors);

  function showFormErrors() {
    return {
      restrict: 'A',
      require: '^form',
      link: function(scope, element, attrs, formCtrl) {
        if (attrs.showFormErrors) {
          scope.$watch(function() {
            return scope.$eval(attrs.showFormErrors);
          }, function(newValue, oldValue) {
            element.toggleClass('has-success',
              oldValue === true && newValue !== true);
            element.toggleClass('has-error', newValue === true);
          });
        } else {
          var oInput = angular.element(element[0].querySelector('[name]'));
          if (!oInput) {
            return;
          }
          var inputName = oInput.attr('name');
          oInput.bind('blur', function() {
            element.toggleClass('has-success', !formCtrl[inputName].$invalid);
            element.toggleClass('has-error', formCtrl[inputName].$invalid);
          });
        }

        scope.$on('form-reset', function() {
          element.removeClass('has-success has-error');
        });
      }
    };
  }

})();
