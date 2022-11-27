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

int calculateCharToSet(int x, int y, bool normaliseX, bool normaliseY, struct winsize termSize, int changeXtoNormalise, int changeYtoNormalise) {
	x *= 2; // double x because 1 char is double as tall as it is wide so 2 charecters makes a sqaure pixel
	
	if (normaliseY) y += changeYtoNormalise; // normalise the pixels - so (0, 0) is the center of the screen
	if (normaliseX) x += changeXtoNormalise; // normalise the pixels - so (0, 0) is the center of the screen

	return ((0 <= x) && (x < termSize.ws_col) && (0 <= y) && (y < termSize.ws_row)) // if our pixel is within screen bounds
		? (y * termSize.ws_col) + x // then return the chareceter we should set
		: -1; // otherwise return -1 which tells our program that the pixel is out of bounds
}

vector <array<int, 2>> getPointsForEighthCircle(int radius) {
	int x = 0;
	int y = radius;
	int d = 3 - 2*radius;
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

class screen {
	private:
		int _noOfScreenChars;
		int _changeYtoNormalise;
		int _changeXtoNormalise;
		struct winsize _termSize;
	public:
		screen(){
			initialiseSize();
		}
		
		u16string _contents;
		int _smallestDimensionSize;

		void initialiseSize(){	
			ioctl(0, TIOCGWINSZ, &_termSize);
			_termSize.ws_row -= 2; // make the screen 2 line less so the terminal prompt and time and screen display can show
			_noOfScreenChars = _termSize.ws_row * _termSize.ws_col;
			switch (_termSize.ws_row < _termSize.ws_col) {
				case true: _smallestDimensionSize = _termSize.ws_row; break;
				case false: _smallestDimensionSize = _termSize.ws_col; break;
			}
			_changeXtoNormalise = floor(_termSize.ws_col/2); // the amount of charecters needed to normalise the screen (so 0, 0 is the centre) in X
			_changeYtoNormalise = floor(_termSize.ws_row/2); // the amount of charecters needed to normalise the screen (so 0, 0 is the centre) in Y
			_contents = std::u16string(_noOfScreenChars, u' ');
		}

		void printMe() {
			wstring_convert<codecvt_utf8<char16_t>, char16_t> converter;
			cout << converter.to_bytes(_contents) << endl;
		}

		void drawText(int x, int y, u16string text, bool normaliseTextX, bool normaliseTextY) {
			int charToSet = calculateCharToSet(x, y, normaliseTextX, normaliseTextY, _termSize, _changeXtoNormalise, _changeYtoNormalise);

			if (normaliseTextX)
				charToSet -= round(text.length()/2);

			if (charToSet != -1) {// if char to set is not -1 (the number used when a pixel will not fit on the screen)
				_contents.replace(charToSet, text.length(), text); // then set our screen pixel
			}
		}

		void setPix(int x, int y, char16_t charecter, bool normalisePixelX, bool normalisePixelY) {
			int charToSet = calculateCharToSet(x, y, normalisePixelX, normalisePixelY, _termSize, _changeXtoNormalise, _changeYtoNormalise);

			if (charToSet != -1) {// if char to set is not -1 (the number used when a pixel will not fit the screen)
				// then set our screen pixel
				_contents[charToSet] = charecter;
				_contents[charToSet + 1] = charecter;
			}
		}

		void drawCirclePixel(int originx, int originy, int xc, int yc, char16_t charecter, bool normaliseX, bool normaliseY) {
			// draws 8 pixels of a circle from 1 pixel see https://lectureloops.com/wp-content/uploads/2021/01/image-5.png explaining this process
			setPix(originx + xc, originy + yc, charecter, normaliseX, normaliseY);
			setPix(originx + xc, originy - yc, charecter, normaliseX, normaliseY);
			setPix(originx - xc, originy + yc, charecter, normaliseX, normaliseY);
			setPix(originx - xc, originy - yc, charecter, normaliseX, normaliseY);
			setPix(originx + yc, originy + xc, charecter, normaliseX, normaliseY);
			setPix(originx + yc, originy - xc, charecter, normaliseX, normaliseY);
			setPix(originx - yc, originy + xc, charecter, normaliseX, normaliseY);
			setPix(originx - yc, originy - xc, charecter, normaliseX, normaliseY);
		}

		void drawCircle(int centerx, int centery, int radius, char16_t charecter, int thickness, bool normaliseCircleX, bool normaliseCircleY) {
			int x;
			int y = radius;
			int d = 3 - 2*radius;

			drawCirclePixel(centerx, centery, x, y, charecter, normaliseCircleX, normaliseCircleY);

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
				drawCirclePixel(centerx, centery, x, y, charecter, normaliseCircleX, normaliseCircleY);
				//int i;
				//while (i < thickness) {
				//	drawCirclePixel(centerx, centery, x, y-i, charecter);
				//	i++;
				//}
			}
		}
		
		void bresignham3D(int x1, int y1, int z1, int x2, int y2, int z2, char16_t newChar, bool normaliseX, bool normaliseY) {
			int dx = abs(x1 - x2);
			int dy = abs(y1 - y2);
			int dz = abs(z1 - z2);
			int xs = copysign(1, int(x2-x1));
			int ys = copysign(1, int(y2-y1));
			int zs = copysign(1, int(z2-z1));

			int p1;
			int p2;

			if (dx >= dy & dx >= dz) { // Driving axis is X-axis"
				setPix(x1, y1, newChar, normaliseX, normaliseY);

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
					setPix(x1, y1, newChar, normaliseX, normaliseY);
				}
			} else if (dy >= dx & dy >= dz) { // Driving axis is Y axis
				setPix(x1, y1, newChar, normaliseX, normaliseY);

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
					setPix(x1, y1, newChar, normaliseX, normaliseY);
				}
			} else if (dz >= dx & dz >= dy) { // Driving axis is Z-axis"
				setPix(x1, y1, newChar, normaliseX, normaliseY);

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
					setPix(x1, y1, newChar, normaliseX, normaliseY);
				}
			} else {
				cout << "Could not find the driving axis";
			}
		}
};

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
	screen myScreen;
	
	myScreen.drawText(0, 1, u"CLOCK", true, false);
	myScreen.drawCircle(0, 0, floor(myScreen._smallestDimensionSize/2), u'█', 3, true, true);
	u16string clockStyle = myScreen._contents;

	vector <array<int, 2>> pointsForEighthOfSecond = getPointsForEighthCircle(round(myScreen._smallestDimensionSize/2.7));
	vector <array<int, 2>> pointsForEighthOfMinute = getPointsForEighthCircle(round(myScreen._smallestDimensionSize/3.2));
	vector <array<int, 2>> pointsForEighthOfHour = getPointsForEighthCircle(round(myScreen._smallestDimensionSize/4));

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
		myScreen.bresignham3D(0, 0, 0, pixelForSecond[0], pixelForSecond[1], 0, u'█', true, true);
		myScreen.bresignham3D(0, 0, 0, pixelForMinute[0], pixelForMinute[1], 0, u'#', true, true);
		myScreen.bresignham3D(0, 0, 0, pixelForHour  [0], pixelForHour  [1], 0, u'=', true, true);

		// print and reset screen
		myScreen.printMe();
		myScreen._contents = clockStyle;

		// print time
		cout << timeInfo->tm_hour;
		cout << ":";
		cout << timeInfo->tm_min;
		cout << ":";
		cout << timeInfo->tm_sec;
		cout << endl;
		
		//pause
		usleep(500000);
	}

	return 0;
}
