using simulation

# Generate realistic telemetry CSV for Unity playback
$csvPath = "D:\x-racing\data\telemetry\unity_state.csv"
$header = "time,distance,speed,rpm,gear,throttle,brake,steer,slipAngle,slipRatio,posX,posY,velX,velY,accX,accY,heading,lateralG,longitudinalG"

$lines = [System.Collections.Generic.List[string]]::new()
$lines.Add($header)

$dt = 1.0 / 60.0
$totalFrames = 3600  # 60 seconds at 60Hz

# Track parameters (oval: 765m straight, 75m radius curves)
$straightLength = 765.0
$curveRadius = 75.0
$trackLength = 2.0 * $straightLength + 2.0 * [Math]::PI * $curveRadius

# Vehicle parameters
$maxSpeed = 80.0  # m/s (288 km/h)
$maxRPM = 7200.0
$idleRPM = 800.0
$gearRatios = @(3.67, 2.0, 1.35, 1.0, 0.85, 0.7)
$finalDrive = 3.44
$wheelRadius = 0.33

$time = 0.0
$distance = 0.0
$speed = 0.0
$gear = 1
$heading = 0.0

for ($frame = 0; $frame < $totalFrames; $frame++) {
    $time = [Math]::Round($frame * $dt, 4)
    
    # Simple driving model - accelerate on straights, brake in corners
    $trackPos = $distance % $trackLength
    
    # Determine if in straight or corner
    $inStraight1 = ($trackPos - 0.0) -lt $straightLength
    $inCurve1 = ($trackPos - $straightLength) -lt ([Math]::PI * $curveRadius)
    $inStraight2 = ($trackPos - $straightLength - [Math]::PI * $curveRadius) -lt $straightLength
    
    $throttle = 0.0
    $brake = 0.0
    $steer = 0.0
    
    if ($inStraight1 -or $inStraight2) {
        # Straight - accelerate
        if ($speed -lt $maxSpeed) {
            $throttle = 0.8
        } else {
            $throttle = 0.3
        }
        $steer = 0.0
    } else {
        # Corner - moderate speed
        $targetSpeed = 35.0
        if ($speed -gt $targetSpeed) {
            $brake = 0.3
            $throttle = 0.0
        } else {
            $throttle = 0.5
        }
        # Steering based on curve direction
        if ($inCurve1) {
            $steer = 0.3  # Right curve
        } else {
            $steer = -0.3  # Left curve
        }
    }
    
    # Update speed
    if ($throttle -gt 0) {
        $speed += 2.0 * $dt * $throttle
    }
    if ($brake -gt 0) {
        $speed -= 5.0 * $dt * $brake
    }
    $speed = [Math]::Max(0.0, [Math]::Min($maxSpeed, $speed))
    
    # Calculate RPM from speed and gear
    $gearIndex = [Math]::Max(0, [Math]::Min($gearRatios.Length - 1, $gear - 1))
    $wheelRPM = ($speed / (2.0 * [Math]::PI * $wheelRadius)) * 60.0
    $rpm = $wheelRPM * $gearRatios[$gearIndex] * $finalDrive
    $rpm = [Math]::Max($idleRPM, [Math]::Min($maxRPM, $rpm))
    
    # Auto gear shift
    if ($rpm -gt ($maxRPM * 0.9) -and $gear -lt $gearRatios.Length) {
        $gear++
    } elseif ($rpm -lt ($idleRPM * 1.5) -and $gear -gt 1) {
        $gear--
    }
    
    # Update distance and heading
    $distance += $speed * $dt
    $heading += $steer * $speed * 0.01 * $dt
    
    # Calculate position on oval track
    $posX = 0.0
    $posY = 0.0
    $d = $distance % $trackLength
    
    if ($d -lt $straightLength) {
        # Straight 1
        $posX = $d
        $posY = 0.0
    } elseif ($d -lt ($straightLength + [Math]::PI * $curveRadius)) {
        # Curve 1 (right)
        $angle = ($d - $straightLength) / $curveRadius - [Math]::PI / 2.0
        $posX = $straightLength + $curveRadius * [Math]::Cos($angle)
        $posY = $curveRadius + $curveRadius * [Math]::Sin($angle)
    } elseif ($d -lt (2.0 * $straightLength + [Math]::PI * $curveRadius)) {
        # Straight 2
        $posX = $straightLength - ($d - $straightLength - [Math]::PI * $curveRadius)
        $posY = 2.0 * $curveRadius
    } else {
        # Curve 2 (left)
        $angle = ($d - 2.0 * $straightLength - [Math]::PI * $curveRadius) / $curveRadius + [Math]::PI / 2.0
        $posX = $curveRadius * [Math]::Cos($angle)
        $posY = $curveRadius + $curveRadius * [Math]::Sin($angle)
    }
    
    # Calculate velocity components
    $velX = $speed * [Math]::Cos($heading)
    $velY = $speed * [Math]::Sin($heading)
    
    # Calculate acceleration
    $accX = $throttle * 3.0 - $brake * 5.0
    $accY = $steer * $speed * 0.1
    
    # Slip angles (simplified)
    $slipAngle = $steer * 0.05
    $slipRatio = ($throttle - $brake) * 0.02
    
    # Lateral and longitudinal G
    $lateralG = $steer * $speed * 0.01 / 9.81
    $longitudinalG = ($throttle - $brake) * 0.3
    
    # Format line
    $line = "{0},{1},{2},{3},{4},{5},{6},{7},{8},{9},{10},{11},{12},{13},{14},{15},{16},{17},{18}" -f `
        $time, `
        [Math]::Round($distance, 2), `
        [Math]::Round($speed, 3), `
        [Math]::Round($rpm, 1), `
        $gear, `
        [Math]::Round($throttle, 2), `
        [Math]::Round($brake, 2), `
        [Math]::Round($steer, 3), `
        [Math]::Round($slipAngle, 4), `
        [Math]::Round($slipRatio, 4), `
        [Math]::Round($posX, 2), `
        [Math]::Round($posY, 2), `
        [Math]::Round($velX, 3), `
        [Math]::Round($velY, 3), `
        [Math]::Round($accX, 3), `
        [Math]::Round($accY, 3), `
        [Math]::Round($heading, 4), `
        [Math]::Round($lateralG, 3), `
        [Math]::Round($longitudinalG, 3)
    
    $lines.Add($line)
}

# Write CSV
$lines | Out-File -FilePath $csvPath -Encoding UTF8
Write-Host "Generated $($lines.Count) lines in $csvPath"
