//
//  FirstViewController.m
//  RemoteUIClient
//
//  Created by Oriol Ferrer Mesià on 11/05/14.
//  Copyright (c) 2014 Oriol Ferrer Mesià. All rights reserved.
//

#import "FirstViewController.h"

@implementation FirstViewController

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil{
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self) {
        // Custom initialization
    }
    return self;
}


- (void)viewDidLoad{
	NSLog(@"viewDidLoad");
    [super viewDidLoad];
	recipeImages = [[NSMutableArray alloc] initWithCapacity:10];
	for(int i = 0; i < 10; i++){
		UIView* paramview = [[[NSBundle mainBundle] loadNibNamed:@"ParamView_ipad" owner:self options:nil] firstObject];
		[recipeImages addObject:paramview];
	}

}


- (void)didReceiveMemoryWarning{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}


- (NSInteger)collectionView:(UICollectionView *)collectionView numberOfItemsInSection:(NSInteger)section{
    return recipeImages.count;
}


- (UICollectionViewCell *)collectionView:(UICollectionView *)collectionView cellForItemAtIndexPath:(NSIndexPath *)indexPath{

    static NSString *identifier = @"Cell";
	UICollectionViewCell *cell = [collectionView dequeueReusableCellWithReuseIdentifier:identifier forIndexPath:indexPath];

	for(id view in [cell subviews]){
		[view removeFromSuperview ];
		NSLog(@"remove %@ from %d",view, indexPath.row);
	}
    [cell addSubview:[recipeImages objectAtIndex:indexPath.row] ];
	NSLog(@"addSubview %@ from %d",[recipeImages objectAtIndex:indexPath.row], indexPath.row);

    return cell;
}

@end
