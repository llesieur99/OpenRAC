// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

#import <UIKit/UIKit.h>
#include "ios/bootstrap.h"
#import "ios/prepare.h"
#import "ios/launcher.h"
#include <cstdlib>

static bool prepared(NSURL* folder) {
    NSFileManager* files = NSFileManager.defaultManager;
    NSDictionary* boot = [files attributesOfItemAtPath:[folder URLByAppendingPathComponent:@"iso_data/rac1/SCUS_971.99"].path error:nil];
    NSDictionary* disc = [files attributesOfItemAtPath:[folder URLByAppendingPathComponent:@"iso_data/rac1/disc.iso"].path error:nil];
    return [boot[NSFileSize] unsignedLongLongValue] == 1383028
        && [disc[NSFileSize] unsignedLongLongValue] == 4214095872ULL
        && [files fileExistsAtPath:[folder URLByAppendingPathComponent:@"port/ntsc/level_00/manifest.json"].path];
}

@interface OpenRACImportController : UIViewController
@property(nonatomic, strong) NSURL* destination;
@property(nonatomic) BOOL ready;
@property(nonatomic, copy) void (^importProgress)(NSString*);
@property(nonatomic, copy) void (^importCompletion)(BOOL, NSString*);
- (void)importSource:(NSURL*)source;
@end

@implementation OpenRACImportController
- (void)viewDidLoad {
    [super viewDidLoad];
    __weak OpenRACImportController* weakSelf = self;
    self.view = openrac_launcher_view(self, self.destination, ^{ weakSelf.ready = YES; },
        ^(NSURL* source, void (^progress)(NSString*), void (^completion)(BOOL, NSString*)) {
            OpenRACImportController* owner = weakSelf;
            owner.importProgress = progress;
            owner.importCompletion = completion;
            [owner importSource:source];
        });
}
- (void)setImportStatus:(NSString*)text {
    NSLog(@"OpenRAC import: %@", text);
    if (self.importProgress) self.importProgress(text);
}
- (void)importSource:(NSURL*)source {
    BOOL access = [source startAccessingSecurityScopedResource];
    NSNumber* isDirectory = nil;
    [source getResourceValue:&isDirectory forKey:NSURLIsDirectoryKey error:nil];
    if (isDirectory.boolValue && !prepared(source)) {
        NSString* message = @"Ce dossier ne contient pas les données préparées de Ratchet & Clank (version américaine). Choisissez le dossier data de votre installation OpenRAC.";
        if (self.importCompletion) {
            self.importCompletion(NO, message);
            self.importCompletion = nil;
            self.importProgress = nil;
        }
        if (access) [source stopAccessingSecurityScopedResource];
        return;
    }
    NSNumber* freeBytes = nil;
    [[self.destination URLByDeletingLastPathComponent] getResourceValue:&freeBytes
        forKey:NSURLVolumeAvailableCapacityForImportantUsageKey error:nil];
    NSNumber* sourceBytes = nil;
    [source getResourceValue:&sourceBytes forKey:NSURLFileSizeKey error:nil];
    const unsigned long long required = isDirectory.boolValue ? 6ULL * 1024 * 1024 * 1024
        : sourceBytes.unsignedLongLongValue + 2ULL * 1024 * 1024 * 1024;
    if (freeBytes && freeBytes.unsignedLongLongValue < required) {
        NSString* message = @"Espace insuffisant pour importer et préparer ce jeu. Libérez de l’espace dans Fichiers, puis réessayez. Aucun fichier existant n’a été modifié.";
        if (self.importCompletion) {
            self.importCompletion(NO, message);
            self.importCompletion = nil;
            self.importProgress = nil;
        }
        if (access) [source stopAccessingSecurityScopedResource];
        return;
    }
    [self setImportStatus:@"Importation du jeu… Gardez l’application ouverte."];
    BOOL previousIdle = UIApplication.sharedApplication.idleTimerDisabled;
    UIApplication.sharedApplication.idleTimerDisabled = YES;
    dispatch_async(dispatch_get_global_queue(QOS_CLASS_USER_INITIATED, 0), ^{
        NSError* error = nil;
        NSURL* staging = [[self.destination URLByDeletingLastPathComponent]
            URLByAppendingPathComponent:[@"Import-" stringByAppendingString:NSUUID.UUID.UUIDString]];
        NSFileManager* files = NSFileManager.defaultManager;
        BOOL copied;
        if (isDirectory.boolValue) {
            copied = [files copyItemAtURL:source toURL:staging error:&error];
        } else {
            NSURL* isoFolder = [staging URLByAppendingPathComponent:@"iso_data/rac1" isDirectory:YES];
            copied = [files createDirectoryAtURL:isoFolder withIntermediateDirectories:YES attributes:nil error:&error];
            if (copied) copied = [files copyItemAtURL:source toURL:[isoFolder URLByAppendingPathComponent:@"disc.iso"] error:&error];
            if (copied && !openrac_import_cancelled()) copied = openrac_prepare_disc(staging, ^(NSString* message) {
                dispatch_async(dispatch_get_main_queue(), ^{ [self setImportStatus:message]; });
            }, &error);
        }
        if (openrac_import_cancelled()) {
            copied = NO;
            error = [NSError errorWithDomain:NSCocoaErrorDomain code:NSUserCancelledError
                userInfo:@{NSLocalizedDescriptionKey:@"Importation annulée. Votre copie précédente est conservée."}];
        }
        if (copied) {
            // Keep any interrupted import recoverable instead of overwriting or deleting it.
            NSURL* previous = nil;
            if ([files fileExistsAtPath:self.destination.path]) {
                previous = [[self.destination URLByDeletingLastPathComponent]
                    URLByAppendingPathComponent:[@"Previous-import-" stringByAppendingString:NSUUID.UUID.UUIDString]];
                copied = [files moveItemAtURL:self.destination toURL:previous error:&error];
            }
            if (copied) {
                copied = [files moveItemAtURL:staging toURL:self.destination error:&error];
                if (!copied && previous) [files moveItemAtURL:previous toURL:self.destination error:nil];
            }
        }
        if (access) [source stopAccessingSecurityScopedResource];
        dispatch_async(dispatch_get_main_queue(), ^{
            if (self.importCompletion) {
                self.importCompletion(copied, error.localizedDescription);
                self.importCompletion = nil;
                self.importProgress = nil;
            } else {
                self.ready = copied;
            }
            NSLog(@"OpenRAC import: %@", copied ? @"Le jeu est prêt." : error.localizedDescription);
            UIApplication.sharedApplication.idleTimerDisabled = previousIdle;
        });
    });
}
@end

namespace openrac::ios {
std::vector<std::string> arguments() {
    @autoreleasepool {
        NSURL* documents = [NSFileManager.defaultManager URLsForDirectory:NSDocumentDirectory inDomains:NSUserDomainMask].firstObject;
        NSString* engine = @OPENRAC_IOS_GAME;
        NSString* game = [engine componentsSeparatedByString:@"-"].firstObject;
        NSURL* data = [documents URLByAppendingPathComponent:[NSString stringWithFormat:@"OpenRAC/active/%@/data",game] isDirectory:YES];
        NSFileManager* files = NSFileManager.defaultManager;
        [files createDirectoryAtURL:[data URLByDeletingLastPathComponent] withIntermediateDirectories:YES attributes:nil error:nil];
        NSURL* previous = [documents URLByAppendingPathComponent:@"data" isDirectory:YES];
        if (![files fileExistsAtPath:data.path] && prepared(previous)) {
            [files moveItemAtURL:previous toURL:data error:nil];
        }
        {
            OpenRACImportController* importer = [[OpenRACImportController alloc] init];
            importer.destination = data;
            UIWindowScene* scene = nil;
            for (UIScene* candidate in UIApplication.sharedApplication.connectedScenes) {
                if ([candidate isKindOfClass:UIWindowScene.class]) {
                    scene = (UIWindowScene*)candidate;
                    break;
                }
            }
            UIWindow* window = scene ? [[UIWindow alloc] initWithWindowScene:scene]
                                     : [[UIWindow alloc] initWithFrame:UIScreen.mainScreen.bounds];
            window.rootViewController = importer;
            [window makeKeyAndVisible];
            while (!importer.ready) {
                @autoreleasepool {
                    [NSRunLoop.currentRunLoop runUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.01]];
                }
            }
            window.hidden = YES;
        }
        std::string root = data.path.UTF8String;
        return {"OpenRAC", "--data", root, "--levels", root + "/port/ntsc", "--window", "--no-mods"};
    }
}
}
