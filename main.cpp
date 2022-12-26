#include <sys/ioctl.h> // to get terminal size
#include <iostream>    // for cout (print to console)
#include <string>      // for u16 string class
#include <cmath>       // to round, floor, ceil, absolute and copysign operations
#include <codecvt>     // to convert u16 string to u8 string for printing
#include <locale>      // to convert u16 string to u8 string for printing
#include <unistd.h>    // to sleep (and usleep)
#include <vector>      // for vectors
#include <ctime>       // for current time of day
#include <array>       // for arrays

using namespace std;

// Returns an array for the pionts in an eighth of a circle
vector <array<int, 2>> getPointsForEighthCircle(int diameter) {
	int x = 0;
	int y = floor(diameter/2);
	int d = 3 - diameter;
	vector<std::array<int, 2>> coOrds;
	coOrds.push_back( {x, y} );

	while (y > x) {
		// increment x
		x++;

		// check for decision parameter and correspondingly update d, x, y
		if (d > 0) {
			y--;
			d += 4*(x-y) + 10;
		} else {
			d += 4*x + 6;
		}

		coOrds.push_back( {x, y} );
	}

	return coOrds;
}

// Allows for pixels to be drawn to a long string that overflows across the terminal
// giving the illusion of 2D.
// Also allows for some basic shapes such as circles and lines to be drawn.
class Screen {
	private:
		int _noOfScreenChars;
		int _changeXtoNormalise;
		struct winsize _termSize;
	public:
		Screen(int changeHeight, char16_t blankContentsChar) {
			initialiseSize(changeHeight, blankContentsChar);
		}
		
		u16string contents;
		int smallestDimensionSize;

		void initialiseSize(int changeHeight, char16_t blankContentsChar) {
			ioctl(0, TIOCGWINSZ, &_termSize);
			_termSize.ws_row += changeHeight; // in some cases you need to change the height of the screen
			_noOfScreenChars = _termSize.ws_row * _termSize.ws_col;
			switch (_termSize.ws_row*2 < _termSize.ws_col) {
				case true: smallestDimensionSize = _termSize.ws_row*2; break;
				case false: smallestDimensionSize = _termSize.ws_col; break;
			}
			_changeXtoNormalise = floor(_termSize.ws_col/2); // the amount of charecters needed to normalise the Screen (so 0, 0 is the centre) in X
			contents = std::u16string(_noOfScreenChars, blankContentsChar);
		}

		void printMe() {
			wstring_convert<codecvt_utf8<char16_t>, char16_t> converter;
			cout << converter.to_bytes(contents) << endl;
		}

		void drawText(int x, int y, u16string text, bool normaliseTextX, bool normaliseTextY) {
			// normalise the pixels - so (0, 0) is the center of the Screen
			if (normaliseTextY) y += _termSize.ws_row; // in y
			if (normaliseTextX) x += _changeXtoNormalise; // in x
      
			// divide y by 2 because pixels have an upper and lower half
			y /= 2;

			// if pixel is within screen bounds, then draw the text
			if ((0 <= x) && (x < _termSize.ws_col) && (0 <= y) && (y < _termSize.ws_row)) {
				int charToSet = (y * _termSize.ws_col) + x;
				if (normaliseTextX)
					charToSet -= floor(text.length()/2);
				contents.replace(charToSet, text.length(), text); // then set our Screen text
			}
		}

		void setPix(int x, int y, bool isOn, bool normalisePixelX, bool normalisePixelY) {
			// normalise the pixels - so (0, 0) is the center of the Screen
			if (normalisePixelY) y += _termSize.ws_row; // in y
			if (normalisePixelX) x += _changeXtoNormalise; // in x

			// calculate if pixel is on lower or upper half
			bool lower = ((y % 2) == 1);

			// divide y by 2 since pixels have an upper and lower half
			y = floor(y/2);

			// if pixel is within screen bounds, then draw the pixel
			if ((0 <= x) && (x < _termSize.ws_col) && (0 <= y) && (y < _termSize.ws_row)) {
				int charToSet = (y * _termSize.ws_col) + x;
				
				if (isOn) {
					if (lower) {
						switch (contents[charToSet]) {
							case u'█': break;
							case u'▀': contents[charToSet] = u'█'; break;
							default: contents[charToSet] = u'▄'; break;
						}
					} else {
						switch (contents[charToSet]) {
							case u'█': break;
							case u'▄': contents[charToSet] = u'█'; break;
							default: contents[charToSet] = u'▀'; break;
						}
					}
				}
				else {
					if (lower) {
						switch (contents[charToSet]) {
							case u' ': break;
							case u'▄': contents[charToSet] = u' '; break;
							default: contents[charToSet] = u'▀'; break;
						}
					} else {
						switch (contents[charToSet]) {
							case u' ': break;
							case u'▀': contents[charToSet] = u' '; break;
							default: contents[charToSet] = u'▄'; break;
						}
					}
				}
			}
		}

		// draws 8 pixels of a circle from 1 pixel
		// see: https://lectureloops.com/wp-content/uploads/2021/01/image-5.png
		void drawCirclePixel(int originx, int originy, int xc, int yc, bool isOn, bool normaliseX, bool normaliseY) {
			setPix(originx + xc, originy + yc, isOn, normaliseX, normaliseY);
			setPix(originx + xc, originy - yc, isOn, normaliseX, normaliseY);
			setPix(originx - xc, originy + yc, isOn, normaliseX, normaliseY);
			setPix(originx - xc, originy - yc, isOn, normaliseX, normaliseY);
			setPix(originx + yc, originy + xc, isOn, normaliseX, normaliseY);
			setPix(originx + yc, originy - xc, isOn, normaliseX, normaliseY);
			setPix(originx - yc, originy + xc, isOn, normaliseX, normaliseY);
			setPix(originx - yc, originy - xc, isOn, normaliseX, normaliseY);
		}

		void drawCircle(int centerx, int centery, int diameter, bool isOn, bool normaliseCircleX, bool normaliseCircleY) {
			int x = 0;
			int y = floor(diameter/2);
			int d = 3 - diameter;

			drawCirclePixel(centerx, centery, x, y, isOn, normaliseCircleX, normaliseCircleY);

			while (y >= x) {
				// increment x
				x++;

				// check for decision parameter and correspondingly update d, x, y
				if (d > 0) {
					y--;
					d += 4*(x-y) + 10;
				} else {
					d += 4*x + 6;
				}

				// draw 8 pixels of the circle
				drawCirclePixel(centerx, centery, x, y, isOn, normaliseCircleX, normaliseCircleY);
			}
		}
		
		void bresignham3D(int x1, int y1, int z1, int x2, int y2, int z2, bool isOn, bool normaliseX, bool normaliseY) {
			int dx = abs(x1 - x2);
			int dy = abs(y1 - y2);
			int dz = abs(z1 - z2);
			int xs = copysign(1, int(x2-x1));
			int ys = copysign(1, int(y2-y1));
			int zs = copysign(1, int(z2-z1));

			int p1;
			int p2;

			if (dx >= dy & dx >= dz) { // Driving axis is X-axis"
				setPix(x1, y1, isOn, normaliseX, normaliseY);

				p1 = 2*dy - dx;
				p2 = 2*dz - dx;
				while (x1 != x2) {
					x1 += xs;
					if (p1 >= 0) {
						y1 += ys;
						p1 -= 2 * dx;
					}
					if (p2 >= 0) {
						z1 += zs;
						p2 -= 2 * dx;
					}
					p1 += 2 * dy;
					p2 += 2 * dz;
					setPix(x1, y1, isOn, normaliseX, normaliseY);
				}
			} else if (dy >= dx & dy >= dz) { // Driving axis is Y axis
				setPix(x1, y1, isOn, normaliseX, normaliseY);

				p1 = 2*dx - dy;
				p2 = 2*dz - dy;
				while (y1 != y2) {
					y1 += ys;
					if (p1 >= 0) {
						x1 += xs;
						p1 -= 2 * dy;
					}
					if (p2 >= 0) {
						z1 += zs;
						p2 -= 2 * dy;
					}
					p1 += 2 * dx;
					p2 += 2 * dz;
					setPix(x1, y1, isOn, normaliseX, normaliseY);
				}
			} else if (dz >= dx & dz >= dy) { // Driving axis is Z-axis"
				setPix(x1, y1, isOn, normaliseX, normaliseY);

				p1 = 2*dy - dz;
				p2 = 2*dx - dz;
				while (z1 != z2) {
					z1 += zs;
					if (p1 >= 0) {
						y1 += ys;
						p1 -= 2 * dz;
					}
					if (p2 >= 0) {
						x1 += xs;
						p2 -= 2 * dz;
					}
					p1 += 2 * dy;
					p2 += 2 * dx;
					setPix(x1, y1, isOn, normaliseX, normaliseY);
				}
			} else {
				cout << "Could not find the driving axis";
			}
		}
};

// Calculates a pixel that lies on nth piont of the circle (piontForCircle)
// based on the vector piontsForEight and the bool clockwise.
array<int, 2> calculatePixel(vector<array<int, 2>> pointsForEighth, int pointForCircle, bool clockwise) {
	int pointOn = pointForCircle % (pointsForEighth.size()-1);
	int eighthOn = floor(pointForCircle / (pointsForEighth.size()-1));
	
	if (clockwise) {
		switch (eighthOn) {
			case 7: return {-pointsForEighth[pointsForEighth.size() - 1 - pointOn][0], -pointsForEighth[pointsForEighth.size() - 1 - pointOn][1]}; break;
			case 6: return {-pointsForEighth[                             pointOn][1], -pointsForEighth[                             pointOn][0]}; break;
			case 5: return {-pointsForEighth[pointsForEighth.size() - 1 - pointOn][1],  pointsForEighth[pointsForEighth.size() - 1 - pointOn][0]}; break;
			case 4: return {-pointsForEighth[                             pointOn][0],  pointsForEighth[                             pointOn][1]}; break;
			case 3: return { pointsForEighth[pointsForEighth.size() - 1 - pointOn][0],  pointsForEighth[pointsForEighth.size() - 1 - pointOn][1]}; break;
			case 2: return { pointsForEighth[                             pointOn][1],  pointsForEighth[                             pointOn][0]}; break;
			case 1: return { pointsForEighth[pointsForEighth.size() - 1 - pointOn][1], -pointsForEighth[pointsForEighth.size() - 1 - pointOn][0]}; break;
			case 0: return { pointsForEighth[                             pointOn][0], -pointsForEighth[                             pointOn][1]}; break;

		}
	}
	else {
		switch (eighthOn) {
			case 0: return {-pointsForEighth[                             pointOn][0], -pointsForEighth[                             pointOn][1]}; break;
			case 1: return {-pointsForEighth[pointsForEighth.size() - 1 - pointOn][1], -pointsForEighth[pointsForEighth.size() - 1 - pointOn][0]}; break;
			case 2: return {-pointsForEighth[                             pointOn][1],  pointsForEighth[                             pointOn][0]}; break;
			case 3: return {-pointsForEighth[pointsForEighth.size() - 1 - pointOn][0],  pointsForEighth[pointsForEighth.size() - 1 - pointOn][1]}; break;
			case 4: return { pointsForEighth[                             pointOn][0],  pointsForEighth[                             pointOn][1]}; break;
			case 5: return { pointsForEighth[pointsForEighth.size() - 1 - pointOn][1],  pointsForEighth[pointsForEighth.size() - 1 - pointOn][0]}; break;
			case 6: return { pointsForEighth[                             pointOn][1], -pointsForEighth[                             pointOn][0]}; break;
			case 7: return { pointsForEighth[pointsForEighth.size() - 1 - pointOn][0], -pointsForEighth[pointsForEighth.size() - 1 - pointOn][1]}; break;
		}
	}
}

int main() {
	Screen myScreen(-1, u'.'); // make the Screen 1 line less so the terminal prompt can show
	bool newPixelsAreOn = true;
	
	myScreen.drawCircle(0, 0, round(myScreen.smallestDimensionSize * 0.9), newPixelsAreOn, true, true);
	myScreen.drawText(0, 0, u"CLOCK", true, false);
	u16string clockStyle = myScreen.contents;

	vector <array<int, 2>> pointsForEighthOfSecond = getPointsForEighthCircle(round(myScreen.smallestDimensionSize * 0.75));
	vector <array<int, 2>> pointsForEighthOfMinute = getPointsForEighthCircle(round(myScreen.smallestDimensionSize * 0.63));
	vector <array<int, 2>> pointsForEighthOfHour = getPointsForEighthCircle(round(myScreen.smallestDimensionSize * 0.5));

	array<int, 2> pixelForHour;
	array<int, 2> pixelForMinute;
	array<int, 2> pixelForSecond;

	struct tm * timeInfo;

	while (true) {
		// get current time
		time_t now = time(0);
		timeInfo = localtime ( &now );
		
		// calculate the piont we need for current time
		int secondOn = floor((timeInfo->tm_sec)  * (pointsForEighthOfSecond.size() - 1) / 7.5);
		int minuteOn = floor((timeInfo->tm_min)  * (pointsForEighthOfMinute.size() - 1) / 7.5);
		int hourOn   = floor((timeInfo->tm_hour % 12) * (pointsForEighthOfHour.size() - 1) / 1.5);

		// calculate the pixel for the hands
		pixelForSecond = calculatePixel(pointsForEighthOfSecond, secondOn, true);
		pixelForMinute = calculatePixel(pointsForEighthOfMinute, minuteOn, true);
		pixelForHour = calculatePixel(pointsForEighthOfHour, hourOn, true);
		
		// draw the hands
		myScreen.bresignham3D(0, 0, 0, pixelForSecond[0], pixelForSecond[1], 0, newPixelsAreOn, true, true);
		myScreen.bresignham3D(0, 0, 0, pixelForMinute[0], pixelForMinute[1], 0, newPixelsAreOn, true, true);
		myScreen.bresignham3D(0, 0, 0, pixelForHour  [0], pixelForHour  [1], 0, newPixelsAreOn, true, true);

		// print and reset Screen
		myScreen.printMe();
		myScreen.contents = clockStyle;
		
		//pause
		usleep(500000);
	}

	return 0;
}
