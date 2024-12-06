using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;

namespace SearchApp.Controls
{
    /// <summary>
    /// SearchResultsControl.xaml 的交互逻辑
    /// </summary>
    public partial class SearchResultsControl : UserControl
    {
        private const int PageSize = 1000;
        private List<string> cachedItems = new List<string>();
        private readonly object cacheLock = new object();
        private bool isLoading = false;
        public SearchResultsControl()
        {
            InitializeComponent();
            ItemsSource = new ObservableCollection<string>();
            ResultsListBox.ItemsSource = ItemsSource;

            // 注册滚动事件
            //ResultsListBox.AddHandler(ScrollViewer.ScrollChangedEvent, new ScrollChangedEventHandler(OnScrollChanged));
        }

        public ObservableCollection<string> ItemsSource { get; private set; }


        // 添加新项
        public void AddItem(string item)
        {
            lock (cacheLock)
            {
                if (ItemsSource.Count < PageSize)
                {
                    // Dispatch the addition to the UI thread
                    Dispatcher.Invoke(() => ItemsSource.Add(item));
                }
                else
                {
                    // Add to the cache on the current (non-UI) thread
                    cachedItems.Add(item);
                }
            }
        }

        // 清空列表
        public void ClearItems()
        {
            lock (cacheLock)
            {
                Dispatcher.Invoke(() => ItemsSource.Clear());
                cachedItems.Clear();
            }
        }

        // 滚动到底部时加载更多数据
        private void OnScrollChanged(object sender, ScrollChangedEventArgs e)
        {
            //var scrollViewer = e.OriginalSource as ScrollViewer;
            //if (scrollViewer != null && scrollViewer.VerticalOffset == scrollViewer.ScrollableHeight)
            //{
            //    isLoading = true; // 设置加载标记，避免重复加载
            //    LoadNextPage();
            //    isLoading = false; // 加载完成后重置标记
            //}
        }

        // 加载下1000条数据
        private void LoadNextPage()
        {
            //lock (cacheLock)
            {
                int itemsToLoad = Math.Min(PageSize, cachedItems.Count);
                var itemsToAdd = cachedItems.Take(itemsToLoad).ToList();

                // Ensure addition happens on the UI thread
                Dispatcher.Invoke(() =>
                {
                    foreach (var item in itemsToAdd)
                    {
                        ItemsSource.Add(item);
                    }
                });

                // Remove loaded items from the cache
                cachedItems.RemoveRange(0, itemsToLoad);
            }
        }
    }
}
