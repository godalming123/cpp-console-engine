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
#include <sstream>     // for string streams

// Returns an array for the pionts in an eighth of a circle
std::vector<std::array<int, 2>> getPointsForEighthCircle(int diameter) {
	int x = 0;
	int y = floor(diameter/2);
	int d = 3 - diameter;
	std::vector<std::array<int, 2>> coOrds;
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

const char16_t upperBlock = u'▀';
const char16_t lowerBlock = u'▄';
const char16_t fullBlock = u'█';
const char16_t noBlock = u' ';

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
		
		std::u16string contents;
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
			std::wstring_convert<std::codecvt_utf8<char16_t>, char16_t> converter;
			std::cout << converter.to_bytes(contents) << std::endl;
		}

		void drawText(int x, int y, std::u16string text, bool normaliseX, bool normaliseY) {
			// normalise the pixels - so (0, 0) is the center of the Screen
			if (normaliseY) y += _termSize.ws_row; // in y
			if (normaliseX) x += _changeXtoNormalise; // in x
      
			// divide y by 2 because pixels have an upper and lower half
			y /= 2;
			
			// loop through every line of text
			std::basic_istringstream<char16_t> ss(text);
			for (std::u16string line; std::getline(ss, line, u'\n');) {
				// if line is within screen bounds, then draw the line
				if ((0 <= x) && (x < _termSize.ws_col) && (0 <= y) && (y < _termSize.ws_row)) {
					int charToSet = (y * _termSize.ws_col) + x; // calculate char to set
					if (normaliseX) // adjust char to set to take into acount text centering
						charToSet -= floor(line.length()/2);
					contents.replace(charToSet, line.length(), line); // then set our Screen text
				}
				y++; // increment y as we have a new line of text
			}
		}

		void moveText(int x1, int y1, int x2, int y2, int width, int height, char16_t newWhitespaceChar, bool normaliseX, bool normaliseY) {
			// normalise the pixels - so (0, 0) is the center of the Screen
			if (normaliseY) y1 += _termSize.ws_row; // in y
			if (normaliseX) x1 += _changeXtoNormalise; // in x

			// cut the text
			std::u16string text;
			std::u16string newWhitespaceText = std::u16string(width, newWhitespaceChar);
			for (int y = y1; y < y1 + height; y++) { // loop through each line of text
				int charToSet = (y * _termSize.ws_col) + x1; // calculate char to set
				text += contents.substr(charToSet, width); // add the line from the text to our variable
				text += u"\n"; // add a newline to our variable
				contents.replace(charToSet, width, newWhitespaceText); // replace the copied text with whitespace
			}

			// paste the text
			drawText(x2, y2, text, normaliseX, normaliseY);
		}
		
		void setChar(int x, int y, char16_t pixelChar, bool normaliseX, bool normaliseY) {
			// normalise the pixels - so (0, 0) is the center of the Screen
			if (normaliseY) y += _termSize.ws_row; // in y
			if (normaliseX) x += _changeXtoNormalise; // in x
			
			// divide y by 2 since pixels have an upper and lower half
			y = floor(y/2);

			if ((0 <= x) && (x < _termSize.ws_col) && (0 <= y) && (y < _termSize.ws_row)) {
				int charToSet = (y * _termSize.ws_col) + x;
				contents[charToSet] = pixelChar;
			}
		}

		void setPix(int x, int y, bool isOn, bool normaliseX, bool normaliseY) {
			// normalise the pixels - so (0, 0) is the center of the Screen
			if (normaliseY) y += _termSize.ws_row; // in y
			if (normaliseX) x += _changeXtoNormalise; // in x

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
							case fullBlock: break;
							case upperBlock: contents[charToSet] = fullBlock; break;
							default: contents[charToSet] = lowerBlock; break;
						}
					} else {
						switch (contents[charToSet]) {
							case fullBlock: break;
							case lowerBlock: contents[charToSet] = fullBlock; break;
							default: contents[charToSet] = upperBlock; break;
						}
					}
				}
				else {
					if (lower) {
						switch (contents[charToSet]) {
							case noBlock: break;
							case lowerBlock: contents[charToSet] = noBlock; break;
							default: contents[charToSet] = upperBlock; break;
						}
					} else {
						switch (contents[charToSet]) {
							case noBlock: break;
							case upperBlock: contents[charToSet] = noBlock; break;
							default: contents[charToSet] = lowerBlock; break;
						}
					}
				}
			}
		}

		// draws 8 pixels of a circle from 1 pixel
		// see: https://lectureloops.com/wp-content/uploads/2021/01/image-5.png
		void drawCirclePixel(int originx, int originy, int xc, int yc, bool isOn, bool normaliseX, bool normaliseY, char16_t circleBackground, bool drawEdge) {
			// if (circleBackground != u'n') {
			// 	std::cout << "before:";
			// 	std::cout << yc;
			// 	std::cout << " ";
			// 	std::cout << xc << std::endl;
			// 	if ((yc % 2) == 0) {
			// 		// we need to set the bottom
			// 		for (int pixelIndex = -xc; pixelIndex < xc+1; pixelIndex++) {setChar(originx + pixelIndex, originy - yc, circleBackground, normaliseX, normaliseY);}
			// 		for (int pixelIndex = -yc; pixelIndex < yc+1; pixelIndex++) {setChar(originx + pixelIndex, originy + xc-1, circleBackground, normaliseX, normaliseY);}
			// 	}
			// 	else {
			// 		// we need to set the top
			// 		for (int pixelIndex = -xc; pixelIndex < xc+1; pixelIndex++) {setChar(originx + pixelIndex, originy + yc, circleBackground, normaliseX, normaliseY);}
			// 		for (int pixelIndex = -yc; pixelIndex < yc+1; pixelIndex++) {setChar(originx - pixelIndex, originy - xc, circleBackground, normaliseX, normaliseY);}
			// 	}
			// 	std::cout << "after:";
			// 	std::cout << yc;
			// 	std::cout << " ";
			// 	std::cout << xc << std::endl;
			// }
			
			if (drawEdge) {
				setPix(originx + xc, originy + yc, isOn, normaliseX, normaliseY);
				setPix(originx + xc, originy - yc, isOn, normaliseX, normaliseY);
				setPix(originx - xc, originy + yc, isOn, normaliseX, normaliseY);
				setPix(originx - xc, originy - yc, isOn, normaliseX, normaliseY);
				setPix(originx + yc, originy + xc, isOn, normaliseX, normaliseY);
				setPix(originx + yc, originy - xc, isOn, normaliseX, normaliseY);
				setPix(originx - yc, originy + xc, isOn, normaliseX, normaliseY);
				setPix(originx - yc, originy - xc, isOn, normaliseX, normaliseY);
			}
		}

		void drawCircle(int centerx, int centery, int diameter, bool isOn, bool normaliseCircleX, bool normaliseCircleY, char16_t circleBackground, bool drawEdge) {
			int x = 0;
			int y = floor(diameter/2);
			int d = 3 - diameter;

			while (y >= x) {
				// draw 8 pixels of the circle
				drawCirclePixel(centerx, centery, x, y, isOn, normaliseCircleX, normaliseCircleY, circleBackground, drawEdge);

				// increment x
				x++;

				// check for decision parameter and correspondingly update d, x, y
				if (d > 0) {
					y--;
					d += 4*(x-y) + 10;
				} else {
					d += 4*x + 6;
				}
			}
		}
		
		void bresignham(int x1, int y1, int x2, int y2, bool isOn, bool normaliseX, bool normaliseY) {
			int dx = abs(x1 - x2);
			int dy = abs(y1 - y2);

			int xs = copysign(1, int(x2-x1));
			int ys = copysign(1, int(y2-y1));

			int p1;
			int p2;

			if (dx >= dy) { // Driving axis is X-axis
				setPix(x1, y1, isOn, normaliseX, normaliseY);

				int p = 2*dy - dx;
				while (x1 != x2) {
					x1 += xs;
					if (p >= 0) {
						y1 += ys;
						p -= 2 * dx;
					}
					p += 2 * dy;
					setPix(x1, y1, isOn, normaliseX, normaliseY);
				}
			} else if (dy >= dx) { // Driving axis is Y axis
				setPix(x1, y1, isOn, normaliseX, normaliseY);

				int p = 2*dx - dy;
				while (y1 != y2) {
					y1 += ys;
					if (p >= 0) {
						x1 += xs;
						p -= 2 * dy;
					}
					p += 2 * dx;
					setPix(x1, y1, isOn, normaliseX, normaliseY);
				}
			} else {
				std::cout << "Could not find the driving axis";
			}
		}
};

// Calculates a pixel that lies on nth piont of the circle (piontForCircle)
// based on the vector piontsForEight and the bool clockwise.
std::array<int, 2> calculatePixel(std::vector<std::array<int, 2>> pointsForEighth, int pointForCircle, bool clockwise) {
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
	
	myScreen.drawCircle(0, 5, round(myScreen.smallestDimensionSize * 1)-18, newPixelsAreOn, true, true, u'n', true);
	myScreen.drawText(0, 0, uR""""(
An example,
 ██████.██.......██████...██████.██..██
██......██......██....██.██......██.██.
██......██......██....██.██......████..
 ██████..██████..██████...██████.██.███)"""", true, false);
	std::u16string clockStyle = myScreen.contents;

	std::vector <std::array<int, 2>> pointsForEighthOfSecond = getPointsForEighthCircle(round(myScreen.smallestDimensionSize * 0.75)-10);
	std::vector <std::array<int, 2>> pointsForEighthOfMinute = getPointsForEighthCircle(round(myScreen.smallestDimensionSize * 0.63)-10);
	std::vector <std::array<int, 2>> pointsForEighthOfHour = getPointsForEighthCircle(round(myScreen.smallestDimensionSize * 0.5)-10);

	std::array<int, 2> pixelForHour;
	std::array<int, 2> pixelForMinute;
	std::array<int, 2> pixelForSecond;

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
		myScreen.bresignham(0, 4, pixelForSecond[0], pixelForSecond[1]+5, newPixelsAreOn, true, true);
		myScreen.bresignham(0, 4, pixelForMinute[0], pixelForMinute[1]+5, newPixelsAreOn, true, true);
		myScreen.bresignham(0, 4, pixelForHour  [0], pixelForHour  [1]+5, newPixelsAreOn, true, true);

		// print and reset Screen
		myScreen.printMe();
		myScreen.contents = clockStyle;
		
		//pause
		usleep(500000);
	}
	
	return 0;
}
