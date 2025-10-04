import { useAMRFleet, useWarehouseAnalytics } from '../hooks/useWarehouseData';

// Sidebar Component
const Sidebar = () => {
  return (
    <aside className="fixed inset-y-0 flex-wrap items-center justify-between block w-full p-0 my-4 overflow-y-auto antialiased transition-transform duration-200 -translate-x-full bg-white border-0 shadow-xl dark:shadow-none dark:bg-slate-850 max-w-64 ease-nav-brand z-990 xl:ml-6 rounded-2xl xl:left-0 xl:translate-x-0" aria-expanded="false">
      <div className="h-19">
        <a className="block px-8 py-6 m-0 text-sm whitespace-nowrap dark:text-white text-slate-700" href="#" target="_blank">
          <img src="/assets/img/logo-ct-dark.png" className="inline h-full max-w-full transition-all duration-200 dark:hidden ease-nav-brand max-h-8" alt="main_logo" />
          <img src="/assets/img/logo-ct.png" className="hidden h-full max-w-full transition-all duration-200 dark:inline ease-nav-brand max-h-8" alt="main_logo" />
          <span className="ml-1 font-semibold transition-all duration-200 ease-nav-brand">Fleet Manager</span>
        </a>
      </div>

      <hr className="h-px mt-0 bg-transparent bg-gradient-to-r from-transparent via-black/40 to-transparent dark:bg-gradient-to-r dark:from-transparent dark:via-white dark:to-transparent" />

      <div className="items-center block w-auto max-h-screen overflow-auto h-sidenav grow basis-full">
        <ul className="flex flex-col pl-0 mb-0">
          <li className="mt-0.5 w-full">
            <a className="py-2.7 bg-blue-500/13 dark:text-white dark:opacity-80 text-sm ease-nav-brand my-0 mx-2 flex items-center whitespace-nowrap rounded-lg px-4 font-semibold text-slate-700 transition-colors" href="#dashboard">
              <div className="mr-2 flex h-8 w-8 items-center justify-center rounded-lg bg-center stroke-0 text-center xl:p-2.5">
                <i className="relative top-0 text-sm leading-normal text-blue-500 ni ni-tv-2"></i>
              </div>
              <span className="ml-1 duration-300 opacity-100 pointer-events-none ease">Dashboard</span>
            </a>
          </li>

          <li className="mt-0.5 w-full">
            <a className="dark:text-white dark:opacity-80 py-2.7 text-sm ease-nav-brand my-0 mx-2 flex items-center whitespace-nowrap px-4 transition-colors" href="#fleet">
              <div className="mr-2 flex h-8 w-8 items-center justify-center rounded-lg bg-center stroke-0 text-center xl:p-2.5">
                <i className="relative top-0 text-sm leading-normal text-orange-500 fas fa-robot"></i>
              </div>
              <span className="ml-1 duration-300 opacity-100 pointer-events-none ease">Fleet Status</span>
            </a>
          </li>

          <li className="mt-0.5 w-full">
            <a className="dark:text-white dark:opacity-80 py-2.7 text-sm ease-nav-brand my-0 mx-2 flex items-center whitespace-nowrap px-4 transition-colors" href="#zones">
              <div className="mr-2 flex h-8 w-8 items-center justify-center rounded-lg bg-center fill-current stroke-0 text-center xl:p-2.5">
                <i className="relative top-0 text-sm leading-normal text-emerald-500 fas fa-warehouse"></i>
              </div>
              <span className="ml-1 duration-300 opacity-100 pointer-events-none ease">Warehouse Zones</span>
            </a>
          </li>

          <li className="mt-0.5 w-full">
            <a className="dark:text-white dark:opacity-80 py-2.7 text-sm ease-nav-brand my-0 mx-2 flex items-center whitespace-nowrap px-4 transition-colors" href="#alerts">
              <div className="mr-2 flex h-8 w-8 items-center justify-center rounded-lg bg-center stroke-0 text-center xl:p-2.5">
                <i className="relative top-0 text-sm leading-normal text-red-600 fas fa-exclamation-triangle"></i>
              </div>
              <span className="ml-1 duration-300 opacity-100 pointer-events-none ease">Alerts</span>
            </a>
          </li>

          <li className="w-full mt-4">
            <h6 className="pl-6 ml-2 text-xs font-bold leading-tight uppercase dark:text-white opacity-60">System</h6>
          </li>

          <li className="mt-0.5 w-full">
            <a className="dark:text-white dark:opacity-80 py-2.7 text-sm ease-nav-brand my-0 mx-2 flex items-center whitespace-nowrap px-4 transition-colors" href="#settings">
              <div className="mr-2 flex h-8 w-8 items-center justify-center rounded-lg bg-center stroke-0 text-center xl:p-2.5">
                <i className="relative top-0 text-sm leading-normal text-slate-700 fas fa-cog"></i>
              </div>
              <span className="ml-1 duration-300 opacity-100 pointer-events-none ease">Settings</span>
            </a>
          </li>
        </ul>
      </div>
    </aside>
  );
};

// Navbar Component
const Navbar = () => {
  return (
    <nav className="relative flex flex-wrap items-center justify-between px-0 py-2 mx-6 transition-all ease-in shadow-none duration-250 rounded-2xl lg:flex-nowrap lg:justify-start" navbar-main="true" navbar-scroll="false">
      <div className="flex items-center justify-between w-full px-4 py-1 mx-auto flex-wrap-inherit">
        <nav>
          <ol className="flex flex-wrap pt-1 mr-12 bg-transparent rounded-lg sm:mr-16">
            <li className="text-sm leading-normal">
              <a className="text-white opacity-50" href="javascript:;">Fleet Management</a>
            </li>
            <li className="text-sm pl-2 capitalize leading-normal text-white before:float-left before:pr-2 before:text-white before:content-['/']" aria-current="page">Dashboard</li>
          </ol>
          <h6 className="mb-0 font-bold text-white capitalize">Fleet Manager Dashboard</h6>
        </nav>

        <div className="flex items-center mt-2 grow sm:mt-0 sm:mr-6 md:mr-0 lg:flex lg:basis-auto">
          <div className="flex items-center md:ml-auto md:pr-4">
            <div className="relative flex flex-wrap items-stretch w-full transition-all rounded-lg ease">
              <span className="text-sm ease leading-5.6 absolute z-50 -ml-px flex h-full items-center whitespace-nowrap rounded-lg rounded-tr-none rounded-br-none border border-r-0 border-transparent bg-transparent py-2 px-2.5 text-center font-normal text-slate-500 transition-all">
                <i className="fas fa-search"></i>
              </span>
              <input type="text" className="pl-9 text-sm focus:shadow-primary-outline ease w-1/100 leading-5.6 relative -ml-px block min-w-0 flex-auto rounded-lg border border-solid border-gray-300 dark:bg-slate-850 dark:text-white bg-white bg-clip-padding py-2 pr-3 text-gray-700 transition-all placeholder:text-gray-500 focus:border-blue-500 focus:outline-none focus:transition-shadow" placeholder="Search robots..." />
            </div>
          </div>
          <ul className="flex flex-row justify-end pl-0 mb-0 list-none md-max:w-full">
            <li className="flex items-center">
              <a href="#" className="block px-0 py-2 text-sm font-semibold text-white transition-all ease-nav-brand">
                <i className="fa fa-user sm:mr-1"></i>
                <span className="hidden sm:inline">Admin</span>
              </a>
            </li>
            <li className="flex items-center px-4">
              <a href="javascript:;" className="p-0 text-sm text-white transition-all ease-nav-brand">
                <i className="cursor-pointer fa fa-cog"></i>
              </a>
            </li>
            <li className="relative flex items-center pr-2">
              <a href="javascript:;" className="block p-0 text-sm text-white transition-all ease-nav-brand">
                <i className="cursor-pointer fa fa-bell"></i>
              </a>
            </li>
          </ul>
        </div>
      </div>
    </nav>
  );
};

// Stats Card Component (Authentic Argon Style)
const StatsCard = ({ title, value, change, changeType, icon, iconColor }: {
  title: string;
  value: string | number;
  change: string;
  changeType: 'positive' | 'negative';
  icon: string;
  iconColor: string;
}) => {
  return (
    <div className="w-full max-w-full px-3 mb-6 sm:w-1/2 sm:flex-none xl:mb-0 xl:w-1/4">
      <div className="relative flex flex-col min-w-0 break-words bg-white shadow-xl dark:bg-slate-850 dark:shadow-dark-xl rounded-2xl bg-clip-border">
        <div className="flex-auto p-4">
          <div className="flex flex-row -mx-3">
            <div className="flex-none w-2/3 max-w-full px-3">
              <div>
                <p className="mb-0 font-sans text-sm font-semibold leading-normal uppercase dark:text-white dark:opacity-60">{title}</p>
                <h5 className="mb-2 font-bold dark:text-white">{value}</h5>
                <p className="mb-0 dark:text-white dark:opacity-60">
                  <span className={`text-sm font-bold leading-normal ${changeType === 'positive' ? 'text-emerald-500' : 'text-red-500'}`}>
                    {change}
                  </span>
                  since yesterday
                </p>
              </div>
            </div>
            <div className="px-3 text-right basis-1/3">
              <div className={`inline-block w-12 h-12 text-center rounded-circle bg-gradient-to-tl ${iconColor}`}>
                <i className={`ni leading-none ni-money-coins text-lg relative top-3.5 text-white ${icon}`}></i>
              </div>
            </div>
          </div>
        </div>
      </div>
    </div>
  );
};

// Fleet Table Component
const FleetTable = () => {
  const { robots, sendRobotToCharge, sendRobotToMaintenance, assignTask } = useAMRFleet();

  const getStatusBadge = (status: string) => {
    const statusClasses = {
      'Active': 'bg-gradient-to-tl from-emerald-600 to-lime-400 px-2.5 text-xs rounded-1.8 py-1.4 inline-block whitespace-nowrap text-center align-baseline font-bold uppercase leading-none text-white',
      'Idle': 'bg-gradient-to-tl from-slate-600 to-slate-300 px-2.5 text-xs rounded-1.8 py-1.4 inline-block whitespace-nowrap text-center align-baseline font-bold uppercase leading-none text-white',
      'Charging': 'bg-gradient-to-tl from-orange-600 to-yellow-400 px-2.5 text-xs rounded-1.8 py-1.4 inline-block whitespace-nowrap text-center align-baseline font-bold uppercase leading-none text-white',
      'Maintenance': 'bg-gradient-to-tl from-purple-700 to-pink-500 px-2.5 text-xs rounded-1.8 py-1.4 inline-block whitespace-nowrap text-center align-baseline font-bold uppercase leading-none text-white',
      'Error': 'bg-gradient-to-tl from-red-600 to-orange-600 px-2.5 text-xs rounded-1.8 py-1.4 inline-block whitespace-nowrap text-center align-baseline font-bold uppercase leading-none text-white'
    };
    return statusClasses[status as keyof typeof statusClasses] || statusClasses.Idle;
  };

  return (
    <div className="flex flex-wrap -mx-3">
      <div className="flex-none w-full max-w-full px-3">
        <div className="relative flex flex-col min-w-0 mb-6 break-words bg-white border-0 border-transparent border-solid shadow-xl dark:bg-slate-850 dark:shadow-dark-xl rounded-2xl bg-clip-border">
          <div className="p-6 pb-0 mb-0 border-b-0 border-b-solid rounded-t-2xl border-b-transparent">
            <h6 className="dark:text-white">AMR Fleet Status</h6>
          </div>
          <div className="flex-auto px-0 pt-0 pb-2">
            <div className="p-0 overflow-x-auto">
              <table className="items-center w-full mb-0 align-top border-gray-200 text-slate-500">
                <thead className="align-bottom">
                  <tr>
                    <th className="px-6 py-3 font-bold text-left uppercase align-middle bg-transparent border-b border-gray-200 shadow-none text-xxs border-b-solid tracking-none whitespace-nowrap text-slate-400 opacity-70">Robot</th>
                    <th className="px-6 py-3 pl-2 font-bold text-left uppercase align-middle bg-transparent border-b border-gray-200 shadow-none text-xxs border-b-solid tracking-none whitespace-nowrap text-slate-400 opacity-70">Status</th>
                    <th className="px-6 py-3 font-bold text-center uppercase align-middle bg-transparent border-b border-gray-200 shadow-none text-xxs border-b-solid tracking-none whitespace-nowrap text-slate-400 opacity-70">Battery</th>
                    <th className="px-6 py-3 font-bold text-center uppercase align-middle bg-transparent border-b border-gray-200 shadow-none text-xxs border-b-solid tracking-none whitespace-nowrap text-slate-400 opacity-70">Tasks</th>
                    <th className="px-6 py-3 font-bold text-center uppercase align-middle bg-transparent border-b border-gray-200 shadow-none text-xxs border-b-solid tracking-none whitespace-nowrap text-slate-400 opacity-70">Efficiency</th>
                    <th className="px-6 py-3 font-semibold capitalize align-middle bg-transparent border-b border-gray-200 border-solid shadow-none tracking-none whitespace-nowrap text-slate-400 opacity-70"></th>
                  </tr>
                </thead>
                <tbody>
                  {robots.map((robot) => (
                    <tr key={robot.id}>
                      <td className="pl-4 pr-6 py-3 align-middle bg-transparent border-b whitespace-nowrap shadow-transparent">
                        <div className="flex px-2 py-1">
                          <div>
                            <img src="/assets/img/small-logos/logo-xd.svg" className="inline-flex items-center justify-center mr-4 text-sm text-white transition-all duration-200 ease-in-out h-9 w-9 rounded-xl" alt="xd" />
                          </div>
                          <div className="flex flex-col justify-center">
                            <h6 className="mb-0 text-sm leading-normal dark:text-white">{robot.name}</h6>
                            <p className="mb-0 text-xs leading-tight text-slate-400">{robot.id}</p>
                          </div>
                        </div>
                      </td>
                      <td className="p-2 align-middle bg-transparent border-b whitespace-nowrap shadow-transparent">
                        <span className={getStatusBadge(robot.status)}>{robot.status}</span>
                        {robot.currentTask && (
                          <p className="text-xs leading-tight text-slate-400 mt-2">{robot.currentTask}</p>
                        )}
                      </td>
                      <td className="p-2 text-center align-middle bg-transparent border-b whitespace-nowrap shadow-transparent">
                        <div className="flex items-center justify-center">
                          <span className="mr-2 text-xs font-semibold leading-tight text-slate-400">{robot.battery}%</span>
                          <div className="text-xs h-0.75 w-30 bg-gray-200 rounded overflow-hidden">
                            <div className={`duration-600 ease-in-out h-0.75 transition-all ${robot.battery > 60 ? 'bg-gradient-to-r from-emerald-400 to-emerald-600' : robot.battery > 30 ? 'bg-gradient-to-r from-orange-400 to-orange-600' : 'bg-gradient-to-r from-red-400 to-red-600'}`} style={{ width: `${robot.battery}%` }}></div>
                          </div>
                        </div>
                      </td>
                      <td className="p-2 text-center align-middle bg-transparent border-b whitespace-nowrap shadow-transparent">
                        <span className="text-xs font-semibold leading-tight text-slate-400">{robot.tasksCompleted}</span>
                      </td>
                      <td className="p-2 text-center align-middle bg-transparent border-b whitespace-nowrap shadow-transparent">
                        <span className="text-xs font-semibold leading-tight text-slate-400">{robot.efficiency}%</span>
                      </td>
                      <td className="p-2 align-middle bg-transparent border-b whitespace-nowrap shadow-transparent">
                        <div className="flex items-center justify-center space-x-2">
                          {robot.status === 'Idle' && (
                            <button
                              onClick={() => assignTask(robot.id, 'New picking task')}
                              className="inline-block px-4 py-2 mb-0 text-xs font-bold text-center uppercase align-middle transition-all bg-transparent border border-solid rounded-lg shadow-none cursor-pointer leading-pro ease-in bg-150 hover:-translate-y-px active:opacity-85 hover:shadow-xs text-emerald-500 border-emerald-500 hover:bg-transparent hover:text-emerald-500 hover:shadow-none active:bg-emerald-500 active:text-white active:hover:bg-transparent active:hover:text-emerald-500"
                            >
                              Assign Task
                            </button>
                          )}
                          {robot.status !== 'Charging' && robot.battery < 50 && (
                            <button
                              onClick={() => sendRobotToCharge(robot.id)}
                              className="inline-block px-4 py-2 mb-0 text-xs font-bold text-center uppercase align-middle transition-all bg-transparent border border-solid rounded-lg shadow-none cursor-pointer leading-pro ease-in bg-150 hover:-translate-y-px active:opacity-85 hover:shadow-xs text-orange-500 border-orange-500 hover:bg-transparent hover:text-orange-500 hover:shadow-none active:bg-orange-500 active:text-white active:hover:bg-transparent active:hover:text-orange-500"
                            >
                              Charge
                            </button>
                          )}
                          {robot.status !== 'Maintenance' && (
                            <button
                              onClick={() => sendRobotToMaintenance(robot.id)}
                              className="inline-block px-4 py-2 mb-0 text-xs font-bold text-center uppercase align-middle transition-all bg-transparent border border-solid rounded-lg shadow-none cursor-pointer leading-pro ease-in bg-150 hover:-translate-y-px active:opacity-85 hover:shadow-xs text-slate-500 border-slate-500 hover:bg-transparent hover:text-slate-500 hover:shadow-none active:bg-slate-500 active:text-white active:hover:bg-transparent active:hover:text-slate-500"
                            >
                              Maintenance
                            </button>
                          )}
                        </div>
                      </td>
                    </tr>
                  ))}
                </tbody>
              </table>
            </div>
          </div>
        </div>
      </div>
    </div>
  );
};

// Main Fleet Dashboard Component
const FleetDashboard = () => {
  const { analytics } = useWarehouseAnalytics();

  return (
    <div className="min-h-screen">
      {/* Blue background */}
      <div className="absolute w-full bg-blue-500 dark:hidden min-h-75"></div>
      
      {/* Sidebar */}
      <Sidebar />

      {/* Main content */}
      <main className="relative h-full max-h-screen transition-all duration-200 ease-in-out xl:ml-68 rounded-xl">
        {/* Navbar */}
        <Navbar />

        {/* Cards */}
        <div className="w-full px-6 py-6 mx-auto">
          {/* Stats Row */}
          <div className="flex flex-wrap -mx-3">
            <StatsCard
              title="Total Robots"
              value={analytics.totalRobots}
              change="+3%"
              changeType="positive"
              icon="fas fa-robot"
              iconColor="from-blue-600 to-cyan-400"
            />
            <StatsCard
              title="Active Robots"
              value={analytics.activeRobots}
              change="+5%"
              changeType="positive"
              icon="fas fa-play"
              iconColor="from-emerald-600 to-lime-400"
            />
            <StatsCard
              title="Avg Battery"
              value={`${analytics.avgBattery}%`}
              change="+12%"
              changeType="positive"
              icon="fas fa-battery-three-quarters"
              iconColor="from-orange-600 to-yellow-400"
            />
            <StatsCard
              title="Fleet Efficiency"
              value={`${analytics.efficiency}%`}
              change="+8%"
              changeType="positive"
              icon="fas fa-chart-line"
              iconColor="from-purple-700 to-pink-500"
            />
          </div>

          {/* Fleet Table */}
          <FleetTable />

        </div>
      </main>
    </div>
  );
};

export default FleetDashboard;