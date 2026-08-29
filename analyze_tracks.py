import math

def analyze_track(name, build_func):
    points = build_func()
    print(f"\n=== {name} ===")
    print(f"Total points: {len(points)}")
    print(f"Start: ({points[0]['x']:.2f}, {points[0]['y']:.2f})")
    print(f"End:   ({points[-1]['x']:.2f}, {points[-1]['y']:.2f})")
    
    # Check closure
    dx = points[-1]['x'] - points[0]['x']
    dy = points[-1]['y'] - points[0]['y']
    dist = math.sqrt(dx*dx + dy*dy)
    print(f"Closure gap: {dist:.3f}m")
    
    # Check for self-intersections (simple check)
    intersections = 0
    for i in range(len(points)-1):
        for j in range(i+2, len(points)-1):
            # Simple distance-based check
            p1 = points[i]
            p2 = points[i+1]
            p3 = points[j]
            p4 = points[j+1]
            # Check if segments are close
            d = min(
                math.sqrt((p1['x']-p3['x'])**2 + (p1['y']-p3['y'])**2),
                math.sqrt((p1['x']-p4['x'])**2 + (p1['y']-p4['y'])**2),
                math.sqrt((p2['x']-p3['x'])**2 + (p2['y']-p3['y'])**2),
                math.sqrt((p2['x']-p4['x'])**2 + (p2['y']-p4['y'])**2),
            )
            if d < 5.0:  # Very close
                intersections += 1
    print(f"Nearby segment pairs: {intersections}")
    
    # Total length
    total_len = 0
    for i in range(1, len(points)):
        dx = points[i]['x'] - points[i-1]['x']
        dy = points[i]['y'] - points[i-1]['y']
        total_len += math.sqrt(dx*dx + dy*dy)
    print(f"Total length: {total_len:.2f}m")
    
    # Box lane stats
    box_lane_count = sum(1 for p in points if p.get('has_box_lane', False))
    print(f"Points with box lane: {box_lane_count}")
    
    return points

def build_default():
    points = []
    L = 300.0
    R = 100.0
    segmentsPerStraight = 120
    segmentsPerCurve = 60
    
    # Straight 1
    for i in range(segmentsPerStraight + 1):
        t = i / segmentsPerStraight
        points.append({
            'x': t * L, 'y': 0,
            'tangent_x': 1.0, 'tangent_y': 0.0,
            'has_box_lane': i > segmentsPerStraight * 0.1 and i < segmentsPerStraight * 0.9
        })
    
    # Right semicircle
    cx, cy = L, -R
    for i in range(1, segmentsPerCurve + 1):
        t = i / segmentsPerCurve
        angle = math.pi/2 - t * math.pi
        points.append({
            'x': cx + R * math.cos(angle),
            'y': cy + R * math.sin(angle),
            'has_box_lane': False
        })
    
    # Straight 2
    for i in range(1, segmentsPerStraight + 1):
        t = i / segmentsPerStraight
        points.append({
            'x': L - t * L, 'y': -2*R,
            'has_box_lane': False
        })
    
    # Left semicircle
    cx, cy = 0, -R
    for i in range(1, segmentsPerCurve + 1):
        t = i / segmentsPerCurve
        angle = -math.pi/2 - t * math.pi
        points.append({
            'x': cx + R * math.cos(angle),
            'y': cy + R * math.sin(angle),
            'has_box_lane': False
        })
    
    return points

def build_pit():
    points = []
    L = 500.0
    R = 90.0
    segmentsPerStraight = 120
    segmentsPerCurve = 60
    
    # Main straight
    for i in range(segmentsPerStraight + 1):
        t = i / segmentsPerStraight
        points.append({
            'x': t * L, 'y': 0,
            'has_box_lane': False
        })
    
    # Right semicircle
    cx, cy = L, -R
    for i in range(1, segmentsPerCurve + 1):
        t = i / segmentsPerCurve
        angle = math.pi/2 - t * math.pi
        points.append({
            'x': cx + R * math.cos(angle),
            'y': cy + R * math.sin(angle),
            'has_box_lane': False
        })
    
    # Pit straight
    for i in range(1, segmentsPerStraight + 1):
        t = i / segmentsPerStraight
        points.append({
            'x': L - t * L, 'y': -2*R,
            'has_box_lane': True
        })
    
    # Left semicircle
    cx, cy = 0, -R
    for i in range(1, segmentsPerCurve + 1):
        t = i / segmentsPerCurve
        angle = -math.pi/2 - t * math.pi
        points.append({
            'x': cx + R * math.cos(angle),
            'y': cy + R * math.sin(angle),
            'has_box_lane': False
        })
    
    return points

def build_custom():
    points = []
    L1 = 450.0
    R = 130.0
    L2 = 450.0
    segmentsPerStraight = 120
    segmentsPerCurve = 60
    
    # Main straight
    for i in range(segmentsPerStraight + 1):
        t = i / segmentsPerStraight
        points.append({
            'x': t * L1, 'y': 0,
            'has_box_lane': False
        })
    
    # Right hairpin
    cx, cy = L1, -R
    for i in range(1, segmentsPerCurve + 1):
        t = i / segmentsPerCurve
        angle = math.pi/2 - t * math.pi
        points.append({
            'x': cx + R * math.cos(angle),
            'y': cy + R * math.sin(angle),
            'has_box_lane': False
        })
    
    # Pit straight
    for i in range(1, segmentsPerStraight + 1):
        t = i / segmentsPerStraight
        points.append({
            'x': L1 - t * L2, 'y': -2*R,
            'has_box_lane': True
        })
    
    # Left hairpin
    cx, cy = L1 - L2, -R  # (0, -130) with L2=450
    for i in range(1, segmentsPerCurve + 1):
        t = i / segmentsPerCurve
        angle = -math.pi/2 - t * math.pi
        points.append({
            'x': cx + R * math.cos(angle),
            'y': cy + R * math.sin(angle),
            'has_box_lane': False
        })
    
    return points

analyze_track("Default", build_default)
analyze_track("PitCircuit", build_pit)
analyze_track("CustomCircuit", build_custom)
