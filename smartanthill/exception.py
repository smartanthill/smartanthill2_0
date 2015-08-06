# Copyright (C) 2015 OLogN Technologies AG
#
# This source file is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License version 2
# as published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.


class SABaseException(Exception):

    MESSAGE = None

    def __str__(self):  # pragma: no cover
        if self.MESSAGE:
            return self.MESSAGE % self.args
        else:
            return Exception.__str__(self)


class ConfigKeyError(SABaseException, KeyError):

    MESSAGE = "Invalid config data path '%s'"


class LiteMQACKFailed(SABaseException):
    pass


class LiteMQResendFailed(SABaseException):
    pass


class LiteMQUndeclaredExchange(SABaseException):

    MESSAGE = "Exchange is not declared: %s"


class NetworkDataLinkUnsupportedProtocol(SABaseException):

    MESSAGE = "Unsupported Network Data Link protocol '%s'"


class NetworkCommStackServerNotExists(SABaseException):

    MESSAGE = "Communication server does not exist by path '%s'"


class NetworkCommStackServerInternalError(SABaseException):

    MESSAGE = "Internal error. System is to be reinitialized"


class NetworkDataLinkConnectionFailure(SABaseException):

    MESSAGE = "Can't make Data Link using options=%s"


class NetworkRequestCancelled(SABaseException):

    MESSAGE = "Request has been cancelled by user"


class BoardUnknownId(SABaseException):

    MESSAGE = "Unknown board with ID=%s"


class BoardUnknownOperation(SABaseException):

    MESSAGE = "Unknown operation '%s' for %s"


class DeviceUnknownId(SABaseException):

    MESSAGE = "Unknown device with ID=%s"


class DeviceUnknownBoard(SABaseException):

    MESSAGE = "Unknown device board '%s'"


class DeviceUnknownPlugin(SABaseException):

    MESSAGE = "Unknown plugin with ID '%s'"


class APIUnknownRequest(SABaseException):

    MESSAGE = "Unknown '%s' action with key '%s'"


class APIRequiredParams(SABaseException):

    MESSAGE = "These parameters '%s' are required for '%s' action"


class WebRouterMatchNotFound(SABaseException):

    MESSAGE = "Could not find any routes by '%s %s'"
