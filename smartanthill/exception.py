# Copyright (c) 2015, OLogN Technologies AG. All rights reserved.
#
# Redistribution and use of this file in source and compiled
# forms, with or without modification, are permitted
# provided that the following conditions are met:
#     * Redistributions in source form must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#     * Redistributions in compiled form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in the
#       documentation and/or other materials provided with the distribution.
#     * Neither the name of the OLogN Technologies AG nor the names of its
#       contributors may be used to endorse or promote products derived from
#       this software without specific prior written permission.
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL OLogN Technologies AG BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
# DAMAGE


class SABaseException(Exception):

    MESSAGE = None

    def __str__(self):  # pragma: no cover
        if self.MESSAGE:
            return self.MESSAGE % self.args
        else:
            return Exception.__str__(self)


class ConfigKeyError(SABaseException, KeyError):

    MESSAGE = "Invalid config data path '%s'"


class NotImplemnetedYet(SABaseException):
    pass


class LiteMQACKFailed(SABaseException):
    pass


class LiteMQResendFailed(SABaseException):
    pass


class NetworkSATPMessageLost(SABaseException):

    MESSAGE = "Message has been lost: %s"


class NetworkRouterConnectFailure(SABaseException):

    MESSAGE = "Couldn't connect to router with options=%s"


class BoardUnknownId(SABaseException):

    MESSAGE = "Unknown board with ID=%s"


class BoardUnknownOperation(SABaseException):

    MESSAGE = "Unknown operation '%s' for %s"


class DeviceUnknownId(SABaseException):

    MESSAGE = "Unknown device with ID=%s"


class DeviceUnknownBoard(SABaseException):

    MESSAGE = "Unknown device board '%s'"


class DeviceUnknownOperation(SABaseException):

    MESSAGE = "Unknown operation '%s' for device #%d"


class DeviceNotResponding(SABaseException):

    MESSAGE = "Device #%d is not responding (tried %s times)"


class OperArgInvalid(SABaseException):

    MESSAGE = "%s%s: Invalid value '%s'"


class APIUnknownRequest(SABaseException):

    MESSAGE = "Unknown '%s' action with key '%s'"


class APIRequiredParams(SABaseException):

    MESSAGE = "These parameters '%s' are required for '%s' action"


class WebRouterMatchNotFound(SABaseException):

    MESSAGE = "Could not find any routes by '%s %s'"
