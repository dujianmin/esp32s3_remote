'''
Steps to run these cases:
- Build
  - . ${IDF_PATH}/export.sh
  - pip install idf_build_apps
  - python tools/build_apps.py components/usb/usb_stream/test_apps -t esp32s2
- Test
  - pip install -r tools/requirements/requirement.pytest.txt
  - pytest components/usb/usb_stream/test_apps --target esp32s2
'''

import pytest
from pytest_embedded import Dut

@pytest.mark.target('esp32s2')
@pytest.mark.target('esp32s3')
@pytest.mark.env('usb_camera')
@pytest.mark.parametrize(
    'config',
    [
        # Known to cause. assert failed: (rem_len == 0 || is_in)
        # '160mhz',
        '240mhz',
    ],
)
def test_usb_stream(dut: Dut)-> None:
    dut.expect_exact('Press ENTER to see the list of tests.')
    dut.write('[devkit]')
    dut.expect_unity_test_output(timeout = 1000)

@pytest.mark.target('esp32s2')
@pytest.mark.target('esp32s3')
@pytest.mark.env('usb-otg_camera')
@pytest.mark.timeout(60 * 60)
@pytest.mark.parametrize(
    'config',
    [
        # Known to cause. assert failed: (rem_len == 0 || is_in)
        # '160mhz',
        '240mhz',
    ],
)
def test_usb_stream_otg(dut: Dut)-> None:
    dut.expect_exact('Press ENTER to see the list of tests.')
    dut.write('[otg]')
    dut.expect_unity_test_output(timeout = 3000)