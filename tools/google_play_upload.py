#!/usr/bin/python
#
# Copyright 2014 Marta Rodriguez.
#
# Licensed under the Apache License, Version 2.0 (the 'License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""Uploads an apk to the google play store."""

import argparse

from apiclient.discovery import build
import httplib2
from oauth2client import client
from oauth2client.service_account import ServiceAccountCredentials

JSON_KEY='android/Google_Play_Android_Developer-4432a3c4f5d1.json'

# Declare command-line flags.
argparser = argparse.ArgumentParser(add_help=False)
argparser.add_argument('release_track',
                       choices=['alpha', 'beta', 'production', 'rollout'],
                       help='Google play track. Can be alpha, beta, production or rollout')
argparser.add_argument('package_name', 
                       help='The package name. Example: com.android.sample')
argparser.add_argument('apk_file',
                       default='QGroundControl.apk',
                       help='The path to the APK file to upload.')


def main():
  # Create an httplib2.Http object to handle our HTTP requests and authorize it
  # with the Credentials. Note that the first parameter, service_account_name,
  # is the Email address created for the Service account. It must be the email
  # address associated with the key that was created.
  credentials = ServiceAccountCredentials.from_json_keyfile_name(JSON_KEY, scopes=['https://www.googleapis.com/auth/androidpublisher'])

  http = httplib2.Http()
  http = credentials.authorize(http)

  service = build('androidpublisher', 'v3', http=http)

  # Process flags and read their values.
  flags = argparser.parse_args()

  release_track = flags.release_track
  package_name = flags.package_name
  apk_file = flags.apk_file

  try:
    print 'Uploading package %s to track %s' % (package_name, release_track)
    edit_request = service.edits().insert(body={}, packageName=package_name)
    result = edit_request.execute()
    edit_id = result['id']

    apk_response = service.edits().apks().upload(
        editId=edit_id,
        packageName=package_name,
        media_body=apk_file).execute()

    print 'Version code %d has been uploaded' % apk_response['versionCode']

    track_response = service.edits().tracks().update(
        editId=edit_id,
        track=release_track,
        packageName=package_name,
        body={u'releases': [{
            u'versionCodes': [str(apk_response['versionCode'])],
            u'status': u'completed',
        }]}).execute()

    print 'Track %s is set with releases: %s' % (
        track_response['track'], str(track_response['releases']))

    commit_request = service.edits().commit(
        editId=edit_id, packageName=package_name).execute()

    print 'Edit "%s" has been committed' % (commit_request['id'])

  except client.AccessTokenRefreshError:
    print ('The credentials have been revoked or expired, please re-run the '
           'application to re-authorize')

if __name__ == '__main__':
  main()
