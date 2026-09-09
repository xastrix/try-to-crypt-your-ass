# try-to-crypt-your-ass
An attempt to create a loader with decent security that includes basic DLL transmission protection. (unfortunately, I cannot provide you with the full web component of this project; however, I have written a php emulation of the web panel).  
First of all, it is necessary to set up a server; I will use xampp because it is simple:
1. Move all contents from the src/server path to the htdocs folder (xampp)
2. Go to the path `apache/conf/extra/httpd-ssl.conf` and change your settings:
```
DocumentRoot "C:/xampp/htdocs/public"
ServerName localhost:443
```
3. Restart the server
4. Open phpMyAdmin (http://localhost/phpmyadmin) and create a table named 'loader'
5. Compile the project using Visual Studio 2017 and run the client
6. Enter any username and password to initialize the database, then log in again using 'admin' for both fields
## Server requirements
- Your DLL must not weigh more than 10 MB
- You will need an SSL certificate
## License
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
IMPLIED. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY 
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR 
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE 
OR OTHER DEALINGS IN THE SOFTWARE.

1. DISTRIBUTION AND COMMERCIAL USE: You may freely distribute the Software in 
any format, as well as use it for any commercial purposes.
2. MODIFICATION AND ADAPTATION: You may freely modify, adapt, improve, and 
enhance the source code at your discretion, as well as distribute any 
derivative works created based upon it.
3. ATTRIBUTION: You must include the original copyright notice 
and credit to the authors in all copies or substantial portions of the Software, 
as well as in any derivative works.

Copyright (c) 2022 XASTRIX SECURITY REASONS LLC. <xxxastrixxxx@gmail.com> 