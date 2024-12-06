using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using SearchEngineCLI;



namespace SearchApp
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        public MainWindow()
        {
            InitializeComponent();
            InitSearchEngine();
            MySearchControl.SearchClick += OnClick;

        }

        // 事件处理方法
        private void OnClick(string search_content)
        {
            MySearchResultsControl.ItemsSource.Clear();
            loadingText.Text = "搜索中...";
            search_.Search(search_content);
        }

        private void InitSearchEngine()
        {
            search_ = new SearchWrapper();
            search_.Init(OnSearch, OnCompeleted);
        }

        //internal delegate void OnSearchDelegate(string message);
        public void OnSearch(string search_content, bool compelete) { 
            MySearchResultsControl.AddItem(search_content);


            if (compelete) {
                loadingText.Dispatcher.Invoke(() =>
                {
                    loadingText.Text = "搜索完成";
                });
            }

        }

        public void OnCompeleted(bool completed)
        {
            loadingText.Dispatcher.Invoke(() =>
            {
                loadingText.Text = completed ? "加载完成" : "加载失败";
            });
        }

        SearchWrapper search_;
    }
}