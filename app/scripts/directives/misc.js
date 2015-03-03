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

.directive('showFormErrors', function() {
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
})

.directive('loadingContainer', function() {
  return {
    restrict: 'A',
    scope: false,
    link: function(scope, element, attrs) {
      var loadingLayer = angular.element('<div class="loading"></div>');
      element.append(loadingLayer);
      element.addClass('loading-container');
      scope.$watch(attrs.loadingContainer, function(value) {
        loadingLayer.toggleClass('ng-hide', !value);
      });
    }
  };
});
