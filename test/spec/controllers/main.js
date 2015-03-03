'use strict';

describe('Controller: SiteController', function () {

  // load the controller's module
  beforeEach(module('siteApp'));

  var SiteController,
    scope;

  // Initialize the controller and a mock scope
  beforeEach(inject(function ($controller, $rootScope) {
    scope = $rootScope.$new();
    SiteController = $controller('SiteController', {
      $scope: scope
    });
  }));

  // it('should attach a list of awesomeThings to the scope', function () {
  //   expect(scope.awesomeThings.length).toBe(3);
  // });
});
