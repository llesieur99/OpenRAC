// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
// Package the existing project icon as Apple's opaque, full-square iOS source icon.
import AppKit
import Foundation

let source = URL(fileURLWithPath: CommandLine.arguments[1])
let output = URL(fileURLWithPath: CommandLine.arguments[2], isDirectory: true)
let files = FileManager.default
let iconSet = output.appendingPathComponent("AppIcon.appiconset", isDirectory: true)
try files.createDirectory(at: iconSet, withIntermediateDirectories: true)
guard let image = NSImage(contentsOf: source),
      let bitmap = NSBitmapImageRep(bitmapDataPlanes: nil, pixelsWide: 1024, pixelsHigh: 1024,
          bitsPerSample: 8, samplesPerPixel: 3, hasAlpha: false, isPlanar: false,
          colorSpaceName: .deviceRGB, bytesPerRow: 0, bitsPerPixel: 0) else {
    fatalError("Could not read the OpenRAC icon")
}
NSGraphicsContext.saveGraphicsState()
NSGraphicsContext.current = NSGraphicsContext(bitmapImageRep: bitmap)
let rect = NSRect(x: 0, y: 0, width: 1024, height: 1024)
NSGradient(starting: NSColor(srgbRed: 0.95, green: 0.70, blue: 0.15, alpha: 1),
           ending: NSColor(srgbRed: 0.87, green: 0.57, blue: 0.02, alpha: 1))!.draw(in: rect, angle: -90)
image.draw(in: rect, from: .zero, operation: .sourceOver, fraction: 1)
NSGraphicsContext.restoreGraphicsState()
guard let png = bitmap.representation(using: .png, properties: [:]) else {
    fatalError("Could not encode the OpenRAC icon")
}
try png.write(to: iconSet.appendingPathComponent("AppIcon.png"))
let manifest = """
{"images":[{"filename":"AppIcon.png","idiom":"universal","platform":"ios","size":"1024x1024"}],"info":{"author":"xcode","version":1}}
"""
try manifest.write(to: iconSet.appendingPathComponent("Contents.json"), atomically: true, encoding: .utf8)
