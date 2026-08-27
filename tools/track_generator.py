#!/usr/bin/env python3
"""
Track Generator & Parametric Editor
Generates racing track data, modifies geometry, and exports to Unity/UE5 formats
"""

import json
import math
from dataclasses import dataclass, asdict, field
from typing import List, Tuple, Dict, Optional
from pathlib import Path


@dataclass
class Vector3:
    x: float
    y: float
    z: float = 0.0
    
    def to_list(self) -> List[float]:
        return [self.x, self.y, self.z]
    
    def distance_to(self, other: 'Vector3') -> float:
        dx = self.x - other.x
        dy = self.y - other.y
        dz = self.z - other.z
        return math.sqrt(dx*dx + dy*dy + dz*dz)
    
    def __add__(self, other: 'Vector3') -> 'Vector3':
        return Vector3(self.x + other.x, self.y + other.y, self.z + other.z)
    
    def __mul__(self, scalar: float) -> 'Vector3':
        return Vector3(self.x * scalar, self.y * scalar, self.z * scalar)


@dataclass
class TrackSegment:
    """Base class for track segments"""
    id: int
    name: str
    segment_type: str
    start_pos: Vector3
    end_pos: Vector3
    width_m: float = 12.0
    surface: str = "asphalt"
    bank_angle_deg: float = 0.0
    
    def length(self) -> float:
        return self.start_pos.distance_to(self.end_pos)
    
    def midpoint(self) -> Vector3:
        return (self.start_pos + self.end_pos) * 0.5


@dataclass
class CurveSegment(TrackSegment):
    """Curved track segment"""
    radius_inner_m: float = 80.0
    radius_outer_m: float = 95.0
    arc_angle_deg: float = 90.0
    center: Tuple[float, float] = (0, 0)
    curvature_rating: float = 0.5
    apex_speed_kmh: int = 100
    
    def arc_length(self) -> float:
        radius = (self.radius_inner_m + self.radius_outer_m) / 2
        return 2 * math.pi * radius * (self.arc_angle_deg / 360.0)


@dataclass
class Waypoint:
    """Racing line waypoint for AI/lap timing"""
    id: int
    segment_id: int
    position: Vector3
    ideal_line_offset: float = 0.0
    target_speed_kmh: int = 150
    brake_point: bool = False
    corner_number: Optional[int] = None
    is_apex: bool = False


class TrackBuilder:
    """Build and manipulate racing tracks"""
    
    def __init__(self, name: str = "New Track"):
        self.name = name
        self.segments: List[TrackSegment] = []
        self.waypoints: List[Waypoint] = []
        self.pit_lane: Dict = {}
        
    def add_straight(self, id: int, name: str, start: Vector3, end: Vector3, 
                     width: float = 12.0, bank: float = 0.0, surface: str = "asphalt") -> TrackSegment:
        segment = TrackSegment(
            id=id,
            name=name,
            segment_type="straight",
            start_pos=start,
            end_pos=end,
            width_m=width,
            bank_angle_deg=bank,
            surface=surface
        )
        self.segments.append(segment)
        return segment
    
    def add_curve(self, id: int, name: str, center: Tuple[float, float],
                  radius_inner: float, radius_outer: float, arc_angle: float,
                  start_pos: Vector3, end_pos: Vector3, width: float = 12.0,
                  bank: float = 0.0, apex_speed: int = 100, curvature_rating: float = 0.5) -> CurveSegment:
        segment = CurveSegment(
            id=id,
            name=name,
            segment_type="curve",
            start_pos=start_pos,
            end_pos=end_pos,
            width_m=width,
            radius_inner_m=radius_inner,
            radius_outer_m=radius_outer,
            arc_angle_deg=arc_angle,
            center=center,
            bank_angle_deg=bank,
            apex_speed_kmh=apex_speed,
            curvature_rating=curvature_rating
        )
        self.segments.append(segment)
        return segment
    
    def add_waypoint(self, wp: Waypoint):
        self.waypoints.append(wp)
    
    def add_waypoints_from_list(self, data: List[Dict]):
        for wp_data in data:
            wp = Waypoint(
                id=wp_data['id'],
                segment_id=wp_data['segment_id'],
                position=Vector3(*wp_data['position']),
                ideal_line_offset=wp_data.get('ideal_line_offset', 0),
                target_speed_kmh=wp_data.get('target_speed_kmh', 150),
                brake_point=wp_data.get('brake_point', False),
                corner_number=wp_data.get('corner_number'),
                is_apex=wp_data.get('is_apex', False)
            )
            self.add_waypoint(wp)
    
    def calculate_total_length(self) -> float:
        return sum(s.length() for s in self.segments)
    
    def calculate_ideal_lap_time(self, base_time_s: float = 120) -> float:
        total_length = self.calculate_total_length()
        total_complexity = sum(
            getattr(s, 'curvature_rating', 0) for s in self.segments
        )
        complexity_factor = 1.0 + (total_complexity * 0.15)
        return base_time_s * complexity_factor
    
    def export_json(self, filepath: str):
        data = {
            "track_metadata": {
                "name": self.name,
                "length_km": self.calculate_total_length() / 1000.0,
                "ideal_lap_time_s": self.calculate_ideal_lap_time()
            },
            "track_segments": self._serialize_segments(),
            "waypoints": self._serialize_waypoints()
        }
        
        with open(filepath, 'w') as f:
            json.dump(data, f, indent=2)
        print(f"Exported to {filepath}")
    
    def _serialize_segments(self) -> List[Dict]:
        result = []
        for seg in self.segments:
            seg_dict = {
                "id": seg.id,
                "name": seg.name,
                "type": seg.segment_type,
                "width_m": seg.width_m,
                "surface": seg.surface,
                "bank_angle_deg": seg.bank_angle_deg,
            }
            
            if isinstance(seg, TrackSegment):
                seg_dict["start_pos"] = seg.start_pos.to_list()
                seg_dict["end_pos"] = seg.end_pos.to_list()
                seg_dict["length_m"] = seg.length()
            
            if isinstance(seg, CurveSegment):
                seg_dict.update({
                    "center": list(seg.center),
                    "radius_inner_m": seg.radius_inner_m,
                    "radius_outer_m": seg.radius_outer_m,
                    "arc_angle_deg": seg.arc_angle_deg,
                    "apex_speed_kmh": seg.apex_speed_kmh,
                    "curvature_rating": seg.curvature_rating,
                    "arc_length_m": seg.arc_length()
                })
            
            result.append(seg_dict)
        return result
    
    def _serialize_waypoints(self) -> List[Dict]:
        return [
            {
                "id": wp.id,
                "segment_id": wp.segment_id,
                "position": wp.position.to_list(),
                "ideal_line_offset": wp.ideal_line_offset,
                "target_speed_kmh": wp.target_speed_kmh,
                "brake_point": wp.brake_point,
                "corner_number": wp.corner_number,
                "is_apex": wp.is_apex
            }
            for wp in self.waypoints
        ]
    
    def print_summary(self):
        print(f"\n{'='*50}")
        print(f"Track: {self.name}")
        print(f"{'='*50}")
        print(f"Total Length: {self.calculate_total_length():.1f}m ({self.calculate_total_length()/1000:.2f}km)")
        print(f"Estimated Lap Time: {self.calculate_ideal_lap_time():.1f}s")
        print(f"Segments: {len(self.segments)}")
        print(f"Waypoints: {len(self.waypoints)}")
        
        print(f"\nSegments:")
        for seg in self.segments:
            seg_type = seg.segment_type
            length = seg.length()
            print(f"  [{seg.id}] {seg.name:30s} ({seg_type:10s}) - {length:7.1f}m")
        
        if self.waypoints:
            corners = set(wp.corner_number for wp in self.waypoints if wp.corner_number)
            print(f"\nCorners detected: {len(corners)}")
            for corner_id in sorted(corners):
                corner_wps = [wp for wp in self.waypoints if wp.corner_number == corner_id]
                apex_wp = next((wp for wp in corner_wps if wp.is_apex), None)
                if apex_wp:
                    print(f"  Corner {corner_id}: apex speed {apex_wp.target_speed_kmh} km/h")


def load_track_from_json(filepath: str) -> TrackBuilder:
    """Load track from JSON file"""
    with open(filepath, 'r') as f:
        data = json.load(f)
    
    track = TrackBuilder(data['track_metadata'].get('name', 'Loaded Track'))
    track.add_waypoints_from_list(data['waypoints'])
    return track


if __name__ == "__main__":
    builder = TrackBuilder("Test Circuit 2.5km")
    
    builder.add_straight(
        0, "Main Straight",
        Vector3(0, 0, 0),
        Vector3(500, 0, 0),
        width=14.0
    )
    
    builder.add_curve(
        1, "Turn 1 - Slow Right",
        center=(500, -150),
        radius_inner=70,
        radius_outer=84,
        arc_angle=90,
        start_pos=Vector3(500, 0, 0),
        end_pos=Vector3(500, -150, 0),
        apex_speed=90,
        curvature_rating=0.8
    )
    
    builder.add_straight(
        2, "Back Straight",
        Vector3(500, -150, 0),
        Vector3(100, -300, 5),
        width=12.0
    )
    
    builder.add_curve(
        3, "Turn 2 - Fast Left",
        center=(-50, -200),
        radius_inner=150,
        radius_outer=162,
        arc_angle=120,
        start_pos=Vector3(100, -300, 5),
        end_pos=Vector3(-100, -50, 8),
        apex_speed=160,
        curvature_rating=0.4
    )
    
    builder.add_straight(
        4, "Return to Start",
        Vector3(-100, -50, 8),
        Vector3(0, 0, 0),
        width=12.0
    )
    
    waypoints_data = [
        {'id': 0, 'segment_id': 0, 'position': [100, 0, 0], 'target_speed_kmh': 280, 'brake_point': False},
        {'id': 1, 'segment_id': 1, 'position': [500, -30, 0], 'target_speed_kmh': 130, 'brake_point': True, 'corner_number': 1},
        {'id': 2, 'segment_id': 1, 'position': [520, -140, 0], 'target_speed_kmh': 90, 'brake_point': False, 'corner_number': 1, 'is_apex': True},
        {'id': 3, 'segment_id': 2, 'position': [250, -220, 2], 'target_speed_kmh': 220, 'brake_point': False},
        {'id': 4, 'segment_id': 3, 'position': [40, -250, 6], 'target_speed_kmh': 140, 'brake_point': True, 'corner_number': 2},
        {'id': 5, 'segment_id': 3, 'position': [-50, -180, 8], 'target_speed_kmh': 160, 'brake_point': False, 'corner_number': 2, 'is_apex': True},
    ]
    
    builder.add_waypoints_from_list(waypoints_data)
    builder.print_summary()
    builder.export_json('track_generated.json')
    print("\nTrack generation complete!")
