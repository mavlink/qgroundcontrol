-- Calculate COMPASS_CALIBRATED

offsetCalibrated = (getFactValue('P.COMPASS_LEARN') == 1) or
                        isFactNotAtDefault('P.COMPASS_OFS_X') or
                        isFactNotAtDefault('P.COMPASS_OFS_Y') or
                        isFactNotAtDefault('P.COMPASS_OFS_Z')

declinationSet = (getFactValue('P.COMPASS_AUTODEC') == 1) or isFactNotAtDefault('P.COMPASS_DEC')

COMPASS_CALIBRATED  = (offsetCalibrated and declinationSet) and 1 or 0


-- Calculate RC_CALIBRATED

rcCalibrated = false
for i=1,getFactValue('S.RC_CHANNELS_USED') do
    if (isFactNotAtDefault("P.RC" .. i .. "_MIN") or isFactNotAtDefault("P.RC" .. i .. "_MIN") or isFactNotAtDefault("P.RC" .. i .. "_MIN")) then
        rcCalibrated = true
        break
    end
end
RC_CALIBRATED = rcCalibrated and 1 or 0